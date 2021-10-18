# generating private keys

## using bitcoin-explorer

[super useful bitcoin explorer wiki](https://github.com/libbitcoin/libbitcoin-explorer/wiki)

### from mastering bitcoin

note that the examples given in the book use `ec-new` and `ec-to-public`, which are easier as starting examples and for beginners. however, when digging into the source code further down, you'll see `hd` instead of `ec` in some cases (such as `hd_private`). It is sufficient to recognize that `bx` commands for both `ec` and `hd` types exist, and safe to pretend they are the same thing (for now).

private key: `1e99423a4ed27608a15a2616a2b0e9e52ced330ac530edcc32c8ffc6a526aedd`

##### uncompressed public key

```bash
$ bx ec-to-public -u 1e99423a4ed27608a15a2616a2b0e9e52ced330ac530edcc32c8ffc6a526aedd
04f028892bad7ed57d2fb57bf33081d5cfcf6f9ed3d3d7f159c2e2fff579dc341a07cf33da18bd734c600b96a72bbc4749d5141c90ec8ac328ae52ddfe2e505bdb
```
| 1 byte prefix | 32 byte x | 32 byte y |

prefix = `04` (used to indicate uncompressed key)
x = `f028892bad7ed57d2fb57bf33081d5cfcf6f9ed3d3d7f159c2e2fff579dc341a`  
y = `07cf33da18bd734c600b96a72bbc4749d5141c90ec8ac328ae52ddfe2e505bdb`  

##### compressed public key

```bash
$ bx ec-to-public 1e99423a4ed27608a15a2616a2b0e9e52ced330ac530edcc32c8ffc6a526aedd
03f028892bad7ed57d2fb57bf33081d5cfcf6f9ed3d3d7f159c2e2fff579dc341a
```

prefix = `03` (used to indicate compressed key with odd 'y' value) 
x = `f028892bad7ed57d2fb57bf33081d5cfcf6f9ed3d3d7f159c2e2fff579dc341a`

#### diving in

```c++
console_result ec_new::invoke(std::ostream& output, std::ostream& error)
{
    // Bound parameters.
    const data_chunk& seed = get_seed_argument();                   // [0]: libbitcoin namespace type 

    if (seed.size() < minimum_seed_size)
    {
        error << BX_EC_NEW_SHORT_SEED << std::endl;
        return console_result::failure;
    }

    ec_secret secret(new_key(seed));                                // [1]: ec_secret libbitcoin namespace type [2] new key
    if (secret == null_hash)
    {
        error << BX_EC_NEW_INVALID_KEY << std::endl;
        return console_result::failure;
    }

    // We don't use bc::ec_private serialization (WIF) here.
    output << config::ec_private(secret) << std::endl;              // [5]
    return console_result::okay;
}
```

*note that [0,1] are from `libbitcoin-system`*

[0]: from `bitcoin/system/utility/data.hpp`
```c++
template <size_t Size>
using byte_array = std::array<uint8_t, Size>;                       // used in [1,3]

// ...

typedef std::vector<uint8_t> data_chunk;
```

[1]: from `bitcoin/system/math/elliptic_curve.hpp`
```c++
/// Private key:
static BC_CONSTEXPR size_t ec_secret_size = 32;
typedef byte_array<ec_secret_size> ec_secret;
```

[2]: from `libbitcoin-explorer/src/utility.cpp`

```c++
// The key may be invalid, caller may test for null secret.
ec_secret new_key(const data_chunk& seed)
{
    const wallet::hd_private key(seed);                             // [3]
    return key.secret();
}
```

observe that `new_key` returns `byte_array` type but generates an `hd_private` type (hierarchical deterministic wallet) despite this being a simple key generation. the constructor for `wallet::hd_private` has an optional `uint64_t prefixes` param which defaults to `hd_public::mainnet` and invokes a factory function `from_seed(seed, prefixes)`.

in `from_seed`, we will generate an `hd_private` type.

[3]: from `libbitcoin-system/src/wallet/hd_private.cpp`
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

    return hd_private(intermediate.left, intermediate.right, master); // [4]
}
```

[4]: note that `hd_private` extends `hd_public`.

```c++
hd_private::hd_private(const ec_secret& secret,
    const hd_chain_code& chain_code, const hd_lineage& lineage)
  : hd_public(from_secret(secret, chain_code, lineage)),
    secret_(secret)
{
}
```

`config::ec_private` is a simple utility indirection for `encode_base16`

[5]: from `libbitcoin-explorer/src/config/ec_private.cpp`
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


## generating a key pair and address from scratch

yes, yes- we can wear our fancypants and do things like `bx seed | bx ec-new | bx ...` but for demonstration purposes...

generate some entropy
```bash
$ bx seed
fdadee95c17af396bcc264c21299f36c72465abdce1ea10a
```

feed the seed into ec-new, to get a private key. (confirm: order of the curve important here?)
```bash
$ bx ec-new fdadee95c17af396bcc264c21299f36c72465abdce1ea10a
3da4a88efcd38080fcfe22df5d82e859e8343ec52aca800ed997768f0e979c9a
```

// TODO

```bash
$ bx ec-to-public -u 3da4a88efcd38080fcfe22df5d82e859e8343ec52aca800ed997768f0e979c9a
043ec6707e253265ec4e20da552d619f24b311fa157dbe05522a2bd5391cf0b485656300ab179b7552781817becb3485a449c45b3f15d1bc43e1f6d254cac6e50e
```