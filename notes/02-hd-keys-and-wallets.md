# hd heys and wallets

- [hd heys and wallets](#hd-heys-and-wallets)
  - [pre-reqs](#pre-reqs)
  - [overview of hd wallets](#overview-of-hd-wallets)
    - [hmac-sha512](#hmac-sha512)
    - [wallet-input format](#wallet-input-format)
  - [diving in to the code](#diving-in-to-the-code)

## pre-reqs

you should have a basic understanding of what a public/private key pair is. refer to [01-key-pairs.md](01-key-pairs.md) to get up to speed.

## overview of hd wallets

so far we have seen the most basic type of key pair and how they are generated

`entropy -- [ ??? ] --> 256-bit private key -- [ ECDSA ] --> 256-bit public key.`

next, we'll look at hierarchical deterministic wallets and define the missing piece above in the process.

### hmac-sha512

we saw, but brushed over, the use of a function called `hmac_sha512_hash` in [01-key-pairs](01-key-pairs.md). like other hash functions, this takes an input of any length and produces a fixed-length output- in this case 512 bits. 

while it doesn't really matter which method you use for generating a private key (as long as you are consistent)- there are some nice features to using hmac-sha512.

as before, this guide doesn't intend to dive deep on the implementation of this hash algorithm or the reasons for doing so, but we will now define and accept that Bitcoin Core will use this function to convert any entropy into a 512-bit uniformly distributed pattern, if split down the middle we can use the left half as a 256-bit private key with an extra 256-bits left over. if you are generating a single private key, we can safely discard the 2nd half of the 512-bit hash. this is the behavior we saw when using `ec-new` in [01-key-pairs](01-key-pairs.md).

therefore\
`entropy -- [ HMAC-512 ] --> 256-bit private key -- [ ECDSA ] --> 256-bit public key.`

in practice, we don't discard the extra 256 bits. these bits are used as the 'chain code', which is essentially just a bit of randomness that we can use for generating a limitless number of child keys and derived keys that can also generate a limitless number of child keys and so on. anyone familiar with the concept of a 'salt' when hashing passwords can consider this somewhat analogous.

### wallet-input format

while an ec key pair is sufficient for generating an address to interact with the blockchain, hd key chains are much more useful. most modern wallets will use this pattern, and as we saw earlier it is the default behavior of the keys used in Bitcoin Core.

recall we used this function in the previous guide to generate a seed, and then used that seed to generate a private key using `ec-new`

```bash
$ bx seed
fdadee95c17af396bcc264c21299f36c72465abdce1ea10a
```
```bash
$ bx ec-new fdadee95c17af396bcc264c21299f36c72465abdce1ea10a
3da4a88efcd38080fcfe22df5d82e859e8343ec52aca800ed997768f0e979c9a
```

let's now take that seed and use it to generate an hd private key instead
```bash
$ bx hd-new fdadee95c17af396bcc264c21299f36c72465abdce1ea10a
xprv9s21ZrQH143K4X7umFtHTFXC9vgiQ3esb5peMzhoLK7n8x97Hehjtj8o7sC26qVkyKGJDxqXVxi7zLi7dW8GbdUkJcH5RNCUKYKsU8DBBQY
```

okay, wait. this looks totally different. for another thing, it isn't even the right length! to illustrate:

```bash
$ echo -n 3da4a88efcd38080fcfe22df5d82e859e8343ec52aca800ed997768f0e979c9a | wc -c
64
$ echo -n xprv9s21ZrQH143K4X7umFtHTFXC9vgiQ3esb5peMzhoLK7n8x97Hehjtj8o7sC26qVkyKGJDxqXVxi7zLi7dW8GbdUkJcH5RNCUKYKsU8DBBQY | wc -c
111
```

shouldn't we expect to see 128 bytes in the second example? the short answer is no, and it's because `hd-new` writes the output in a totally different format called wallet-input format (WIF) which makes it a little easier to import into other wallets. importing is especially important, as it increases security significantly. we're focusing on technical implmenentation in this guide, so if this has gone over your head, jump on over to some deeper instruction on WIF before continuing.\
[hd wallet overview on learnmeabitcoin.com](https://learnmeabitcoin.com/technical/hd-wallets)\
[WIF on developer.bitcoin.org](https://developer.bitcoin.org/devguide/wallets.html#wallet-import-format-wif)\
[wallet overview from Mastering Bitcoin](https://github.com/bitcoinbook/bitcoinbook/blob/develop/ch05.asciidoc#hd-wallets-bip-32bip-44)

regardless if you understand the concepts above, from here you must accept that an HD wallet private key is written in base58-check encoding. hex encoding (base 16) is case insensitive and is what we see with `ec-new` output (only the secret key, no checksum). base58-check encoding is case sensitive, leaves out some ambiguous characters, and adds a checksum at the end to help protect against misuse. WIF is a superior encoding as it contains much more information and security in a shorter character string.


## diving in to the code

as before, let's start with the individual commandlet from `bitcoin-explorer`, `hd-new`

[0]: from `libbitcoin-explorer/src/commands/hd-new.cpp`
```c++
console_result hd_new::invoke(std::ostream& output, std::ostream& error)
{
    // Bound parameters.
    const auto version = get_version_option();
    const data_chunk& seed = get_seed_argument();

    if (seed.size() < minimum_seed_size)
    {
        error << BX_HD_NEW_SHORT_SEED << std::endl;
        return console_result::failure;
    }

    // We require the private version, but public is unused here.
    const auto prefixes = wallet::hd_private::to_prefixes(version, 0);              // [1] uint64_t prefixes
    const wallet::hd_private private_key(seed, prefixes);                           // [2] libbitcoin type wallet::hd_private

    if (!private_key)
    {
        error << BX_HD_NEW_INVALID_KEY << std::endl;
        return console_result::failure;
    }

    output << private_key << std::endl;                                             // [3] output private key
    return console_result::okay;
}
```

in the `ec-new` example, we saw that `libbitcoin-explorer` used a few helpers to simplify the output because the default behavior of Bitcoin Core uses the more complex (but compatible) `hd_wallet` by default. that helper was called `new_key` and created an `hd_private` type with only the seed (no prefix specified)- which allows the prefix to default to `mainnet`.

[1]: from `libbitcoin-system/include/bitcoin/system/wallet/hd_private.hpp`
```c++
class BC_API hd_private
  : public hd_public
{
public:
    static const uint64_t mainnet;
    static const uint64_t testnet;

// ...

    static uint64_t to_prefixes(uint32_t private_prefix,
        uint32_t public_prefix)
    {
        return uint64_t(private_prefix) << 32 | public_prefix;
    }

    /// Constructors.
    hd_private();
    hd_private(const hd_private& other);
    hd_private(const data_chunk& seed, uint64_t prefixes=mainnet);
// ...
```