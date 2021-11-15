# working with key pairs

- [working with key pairs](#working-with-key-pairs)
  - [pre-reqs](#pre-reqs)
    - [bitcoin-explorer](#bitcoin-explorer)
    - [secp256k1](#secp256k1)
    - [secp256k1 parameters](#secp256k1-parameters)
  - [overview of private / public key pair](#overview-of-private--public-key-pair)
    - [private key from mastering bitcoin](#private-key-from-mastering-bitcoin)
        - [uncompressed public key](#uncompressed-public-key)
        - [compressed public key](#compressed-public-key)
  - [diving in to the code](#diving-in-to-the-code)
- [generating a key pair from scratch](#generating-a-key-pair-from-scratch)

## pre-reqs

this document is an overview of the technical details of the most basic public/private key pairs used in bitcoin

### bitcoin-explorer

`bx` commands used throughout this guide demonstrate use of the bitcoin-explorer command line tool. Bitcoin-explorer uses libbitcoin-system (formerly called libbitcoin), which is the same library bitcoin core uses. Bitcoin-explorer is not included in bitcoin core but can be [cloned from github](https://github.com/libbitcoin/libbitcoin-explorer).

[super useful bitcoin explorer wiki](https://github.com/libbitcoin/libbitcoin-explorer/wiki)

### secp256k1

`secp256k1` is one standard implementation of ECDSA. Parameters defined in this standard must be used when generating or validating any key pairs in Bitcoin.

For more information on `secp256k1`, refer to the [wiki entry on bitcoin.it](https://en.bitcoin.it/wiki/Secp256k1https://en.bitcoin.it/wiki/Secp256k1)

### secp256k1 parameters

note that each parameter is 256 bit, which is easier to see in hex representation.

p = `115792089237316195423570985008687907853269984665640564039457584007908834671663`\
`0x ffff ffff ffff ffff ffff ffff ffff ffff ffff ffff ffff ffff ffff fffe ffff fc2f`

**order**\
n = `115792089237316195423570985008687907852837564279074904382605163141518161494337`\
`0x ffff ffff ffff ffff ffff ffff ffff fffe baae dce6 af48 a03b bfd2 5e8c d036 4141`

**generator**\
x = `55066263022277343669578718895168534326250603453777594175500187360389116729240`\
`0x 79be 667e f9dc bbac 55a0 6295 ce87 0b07 029b fcdb 2dce 28d9 59f2 815b 16f8 1798`

y = `32670510020758816978083085130507043184471273380659243275938904335757337482424`\
`0x 483a da77 26a3 c465 5da4 fbfc 0e11 08a8 fd17 b448 a685 5419 9c47 d08f fb10 d4b8`

## overview of private / public key pair

### private key from mastering bitcoin

note that the examples given in the book use `ec-new` and `ec-to-public`, which are easier as starting examples and for beginners. however, when digging into the source code further down, you'll see `hd` instead of `ec` in some cases (such as `hd_private`). It is sufficient to recognize that `bx` commands for both `ec` and `hd` types exist, and safe to pretend they are the same thing (for now).

private key: `1e99423a4ed27608a15a2616a2b0e9e52ced330ac530edcc32c8ffc6a526aedd`

##### uncompressed public key

```bash
$ bx ec-to-public -u 1e99423a4ed27608a15a2616a2b0e9e52ced330ac530edcc32c8ffc6a526aedd
04f028892bad7ed57d2fb57bf33081d5cfcf6f9ed3d3d7f159c2e2fff579dc341a07cf33da18bd734c600b96a72bbc4749d5141c90ec8ac328ae52ddfe2e505bdb
```
| 1 byte prefix | 32 byte x | 32 byte y |

prefix = `04` (used to indicate uncompressed key)\
x = `f028892bad7ed57d2fb57bf33081d5cfcf6f9ed3d3d7f159c2e2fff579dc341a`\
y = `07cf33da18bd734c600b96a72bbc4749d5141c90ec8ac328ae52ddfe2e505bdb`

##### compressed public key

```bash
$ bx ec-to-public 1e99423a4ed27608a15a2616a2b0e9e52ced330ac530edcc32c8ffc6a526aedd
03f028892bad7ed57d2fb57bf33081d5cfcf6f9ed3d3d7f159c2e2fff579dc341a
```

prefix = `03` (used to indicate compressed key with odd 'y' value)\
x = `f028892bad7ed57d2fb57bf33081d5cfcf6f9ed3d3d7f159c2e2fff579dc341a`

## diving in to the code

[0]: from 
```c++
console_result ec_new::invoke(std::ostream& output, std::ostream& error)
{
    // Bound parameters.
    const data_chunk& seed = get_seed_argument();                   // [1]: libbitcoin namespace type 

    if (seed.size() < minimum_seed_size)
    {
        error << BX_EC_NEW_SHORT_SEED << std::endl;
        return console_result::failure;
    }

    ec_secret secret(new_key(seed));                                // [2]: ec_secret libbitcoin namespace type [2] new key
    if (secret == null_hash)
    {
        error << BX_EC_NEW_INVALID_KEY << std::endl;
        return console_result::failure;
    }

    // We don't use bc::ec_private serialization (WIF) here.
    output << config::ec_private(secret) << std::endl;              // [6]
    return console_result::okay;
}
```

*note that [1,2] are from `libbitcoin-system`*

[1]: from `bitcoin/system/utility/data.hpp`
```c++
template <size_t Size>
using byte_array = std::array<uint8_t, Size>;                       // used in [2,4]

// ...

typedef std::vector<uint8_t> data_chunk;
```

[2]: from `bitcoin/system/math/elliptic_curve.hpp`
```c++
/// Private key:
static BC_CONSTEXPR size_t ec_secret_size = 32;
typedef byte_array<ec_secret_size> ec_secret;
```

[3]: from `libbitcoin-explorer/src/utility.cpp`

```c++
// The key may be invalid, caller may test for null secret.
ec_secret new_key(const data_chunk& seed)
{
    const wallet::hd_private key(seed);                             // [4]
    return key.secret();
}
```

observe that `new_key` returns `byte_array` type but generates an `hd_private` type (hierarchical deterministic wallet) despite this being a simple key generation. the constructor for `wallet::hd_private` has an optional `uint64_t prefixes` param which defaults to `hd_public::mainnet` and invokes a factory function `from_seed(seed, prefixes)`.

in `from_seed`, we will generate an `hd_private` type. Note that is is not important here to understand the purpose of `hmac_sha512_hash` (this will come into play when discussing HD wallets). It is sufficient at this time to understand that `split` will return a struct containing `left`, and `right`, which are both `std::vector<uint8_t>` types. `split` will evenly divide the 512 bit (64 byte) hash into two 256 bit (32 byte) hashes.

[4]: from `libbitcoin-system/src/wallet/hd_private.cpp`
```c++
hd_private hd_private::from_seed(const data_slice& seed, uint64_t prefixes)
{
    // This is a magic constant from BIP32.
    static const auto magic = to_chunk("Bitcoin seed");

    const auto intermediate = split(hmac_sha512_hash(seed, magic));

    // The key is invalid if parse256(IL) >= n or 0:
    if (!verify(intermediate.left))
        return {};

    const auto master = hd_lineage
    {
        prefixes,
        0x00,
        0x00000000,
        0x00000000
    };

    return hd_private(intermediate.left, intermediate.right, master); // [5]
}
```

[5]: note that `hd_private` extends `hd_public`.

Combined with the `hd_private` constructor invocation above, we can see that 256 bits of data passed as the first parameter will correspond to the private key `const ec_secret& secret`. This is first used to generate a public key via `from_secret` to pass to the base class `ec_public` constructor and then stored as a private member of the `hd_private` type. From this, we recognize that the `hd_public` class will not have knowledge of the private key. This design is clever, as an `hd_private` type passed to a function parameter or returned from a function as `hd_public` will be sliced- making the `secret_` member parameter of `hd_private` inaccessible.

```c++
hd_private::hd_private(const ec_secret& secret,
    const hd_chain_code& chain_code, const hd_lineage& lineage)
  : hd_public(from_secret(secret, chain_code, lineage)),
    secret_(secret)
{
}
```

(returning to \[0])
`config::ec_private` is a simple utility indirection for `encode_base16`

[6]: from `libbitcoin-explorer/src/config/ec_private.cpp`
```c++
namespace libbitcoin {
namespace explorer {
namespace config {

// ...

ec_private::ec_private(const ec_secret& secret)
  : value_(secret)
{
}

// ...

std::ostream& operator<<(std::ostream& output, const ec_private& argument)
{
    output << encode_base16(argument.value_);
    return output;
}
```


# generating a key pair from scratch

yes, yes- we can wear our fancypants and do things like `bx seed | bx ec-new | bx ...` but for demonstration purposes...

generate some entropy
```bash
$ bx seed
fdadee95c17af396bcc264c21299f36c72465abdce1ea10a
```

feed the seed into ec-new to get a private key. note that the seed is just a random bit of data, it has nothing to do with the order of the curve or the algorithms defined in `secp256k1` or ECDSA. A seed is always required to generate any type of randomness from a deterministic algorithm. Therefore, it is essential in practice that you use a 'good' source of entropy to generate a seed. 

```bash
$ bx ec-new fdadee95c17af396bcc264c21299f36c72465abdce1ea10a
3da4a88efcd38080fcfe22df5d82e859e8343ec52aca800ed997768f0e979c9a
```

A private key must be in the range [0,n). `ec-new` will report an error if the seed generates an invalid private key, but we can always validate this ourselves to be extra sure.

```python
>>> n = 115792089237316195423570985008687907852837564279074904382605163141518161494337
>>> s = int('3da4a88efcd38080fcfe22df5d82e859e8343ec52aca800ed997768f0e979c9a', 16)
>>> s < n
True
```

```bash
$ bx ec-to-public -u 3da4a88efcd38080fcfe22df5d82e859e8343ec52aca800ed997768f0e979c9a
043ec6707e253265ec4e20da552d619f24b311fa157dbe05522a2bd5391cf0b485656300ab179b7552781817becb3485a449c45b3f15d1bc43e1f6d254cac6e50e
```

prefix = `04`\
x = `3ec6707e253265ec4e20da552d619f24b311fa157dbe05522a2bd5391cf0b485`\
y = `656300ab179b7552781817becb3485a449c45b3f15d1bc43e1f6d254cac6e50e`

for fun, we can validate our point on the curve:\
given `p` and our elliptic curve function as defined in `secp256k1` standard\
`y^2 % p = (x^3 + 7) % p`

Python: 
```python
>>> p = 115792089237316195423570985008687907853269984665640564039457584007908834671663
>>> x = int('3ec6707e253265ec4e20da552d619f24b311fa157dbe05522a2bd5391cf0b485', 16)
>>> y = int('656300ab179b7552781817becb3485a449c45b3f15d1bc43e1f6d254cac6e50e', 16)
>>> x
28394008727450044163285709464316028712584933539979474785302143271295358383237
>>> y
45858520178959979896485123217538564240936697584023300754437863457273837708558
>>> 
>>> 
>>> ((x ** 3) + 7 - (y**2)) % p     # note the inner parenthesis aren't necessary
0
>>>
>>>
>>> ((x ** 3) + 7 - (y**2)) / p     
1.9769753051771942e+152             # yep that's a big number alright
```