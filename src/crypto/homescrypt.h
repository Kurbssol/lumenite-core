#ifndef LITECOIN_CRYPTO_HOMESCRYPT_H
#define LITECOIN_CRYPTO_HOMESCRYPT_H

#include <cstddef>

// Experimental HomeScrypt v1.1 proof-of-work.
// Input is exactly the serialized 80-byte Litecoin-style block header.
// Output is a 32-byte digest.
//
// v1.1 uses a 16 MiB data-dependent scratchpad after an initial
// Scrypt seed stage and reduces ALU-heavy mixing relative to v1. Parameters are experimental and NOT frozen for mainnet.
void homescrypt_v11(const char* input, char* output);

// Exposed for benchmark/reporting only; consensus code should call
// homescrypt_v11 rather than depending on this constant.
std::size_t homescrypt_v11_scratchpad_bytes();

#endif // LITECOIN_CRYPTO_HOMESCRYPT_H
