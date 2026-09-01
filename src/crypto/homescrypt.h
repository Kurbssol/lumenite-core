#ifndef LITECOIN_CRYPTO_HOMESCRYPT_H
#define LITECOIN_CRYPTO_HOMESCRYPT_H

#include <cstddef>
#include <cstdint>

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

// Experimental HomeScrypt v1.2 proof-of-work. Same contract as v1.1: 80 header
// bytes in, a 32-byte digest out, same 16 MiB scratchpad.
//
// v1.2 trades most of v1.1's mix rounds for a dense IEEE-754 binary32 FMA chain
// that gates the next scratchpad address, moving cost off memory latency and
// onto FP32 throughput.
//
// Callers MUST build with FP contraction disabled and fast-math off, or the
// digest will differ between implementations. See doc/homescrypt-v12.md.
//
// Parameters are experimental and NOT frozen for mainnet.
void homescrypt_v12(const char* input, char* output);

// homescrypt_v12 with the tuning parameters supplied by the caller, for
// benchmarks and sweeps. Consensus code must call homescrypt_v12.
void homescrypt_v12_tuned(const char* input, char* output,
                          std::size_t mix_rounds, std::size_t fp_passes);

// Exposed so the tests can assert the never-NaN/infinite/denormal invariant.
float homescrypt_v12_safe_f32(std::uint32_t w);

#endif // LITECOIN_CRYPTO_HOMESCRYPT_H
