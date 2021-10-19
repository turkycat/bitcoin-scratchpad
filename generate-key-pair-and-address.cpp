#include <iostream>
#include <string>
#include <bitcoin/system.hpp>

BC_USE_LIBBITCOIN_MAIN

int bc::main(int argc, char* argv[])
{
    data_chunk entropy(ec_secret_size);


    // Guard against entropy -> invalid secret (rare but possible).
    wallet::ec_private private_key{};
    while (!private_key)
    {
        pseudo_random_fill(entropy);
        private_key = {entropy};
    }

    std::cout << "seed          0x: " << bc::config::base16(entropy) << std::endl;
    std::cout << "secret       b58: " << private_key << std::endl;
    std::cout << "public        0x: " << private_key.to_public() << std::endl;
    std::cout << "address b58check: " << private_key.to_payment_address() << std::endl;

    return 0;
}