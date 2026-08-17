// Experimental HomeScrypt v1.1 proof-of-work for the HomeLite development fork.
// NOT frozen consensus. This implementation is deliberately simple enough to
// audit and port before CPU/GPU optimization work begins.

#include <crypto/homescrypt.h>
#include <crypto/scrypt.h>
#include <crypto/sha256.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {
constexpr std::size_t HEADER_SIZE = 80;
constexpr std::size_t SCRATCHPAD_BYTES = 16 * 1024 * 1024;
constexpr std::size_t WORDS = SCRATCHPAD_BYTES / sizeof(std::uint64_t);
// v1.1 doubles the scratchpad but halves the number of random rounds relative
// to the number of words. This shifts cost from arithmetic toward memory
// capacity/bandwidth while keeping per-hash runtime practical.
constexpr std::size_t RANDOM_MIX_ROUNDS = WORDS / 2;

constexpr unsigned char DOMAIN_SEED[] = {
    'H','O','M','E','S','C','R','Y','P','T','-','V','1','.','1','-','S','E','E','D'
};
constexpr unsigned char DOMAIN_FINAL[] = {
    'H','O','M','E','S','C','R','Y','P','T','-','V','1','.','1','-','F','I','N','A','L'
};

inline std::uint64_t rotl64(std::uint64_t x, unsigned int r)
{
    return (x << r) | (x >> (64 - r));
}

inline std::uint64_t load64le(const unsigned char* p)
{
    return (std::uint64_t)p[0]
        | ((std::uint64_t)p[1] << 8)
        | ((std::uint64_t)p[2] << 16)
        | ((std::uint64_t)p[3] << 24)
        | ((std::uint64_t)p[4] << 32)
        | ((std::uint64_t)p[5] << 40)
        | ((std::uint64_t)p[6] << 48)
        | ((std::uint64_t)p[7] << 56);
}

inline void store64le(unsigned char* p, std::uint64_t x)
{
    p[0] = static_cast<unsigned char>(x);
    p[1] = static_cast<unsigned char>(x >> 8);
    p[2] = static_cast<unsigned char>(x >> 16);
    p[3] = static_cast<unsigned char>(x >> 24);
    p[4] = static_cast<unsigned char>(x >> 32);
    p[5] = static_cast<unsigned char>(x >> 40);
    p[6] = static_cast<unsigned char>(x >> 48);
    p[7] = static_cast<unsigned char>(x >> 56);
}

// SplitMix-style avalanche. This is not intended as the final cryptographic
// primitive by itself; it is used to cheaply diffuse state while forcing the
// memory traffic that dominates HomeScrypt v1.1.
inline std::uint64_t avalanche(std::uint64_t x)
{
    x ^= x >> 30;
    x *= UINT64_C(0xbf58476d1ce4e5b9);
    x ^= x >> 27;
    x *= UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31;
    return x;
}
}

std::size_t homescrypt_v11_scratchpad_bytes()
{
    return SCRATCHPAD_BYTES;
}

void homescrypt_v11(const char* input, char* output)
{
    // Keep a Scrypt-derived seed so the algorithm remains recognizably in the
    // Litecoin/Scrypt family, but make this a small portion of total work.
    std::array<unsigned char, 32> scrypt_seed{};
    scrypt_1024_1_1_256(input, reinterpret_cast<char*>(scrypt_seed.data()));

    std::array<unsigned char, 32> seed{};
    CSHA256()
        .Write(DOMAIN_SEED, sizeof(DOMAIN_SEED))
        .Write(reinterpret_cast<const unsigned char*>(input), HEADER_SIZE)
        .Write(scrypt_seed.data(), scrypt_seed.size())
        .Finalize(seed.data());

    std::array<std::uint64_t, 4> state{{
        load64le(seed.data()),
        load64le(seed.data() + 8),
        load64le(seed.data() + 16),
        load64le(seed.data() + 24)
    }};

    // 16 MiB scratchpad per hashing thread. The sequential fill depends on the
    // previous word, making the complete scratchpad header/nonce-specific.
    std::vector<std::uint64_t> scratch(WORDS);
    std::uint64_t x = state[0] ^ rotl64(state[1], 17)
        ^ rotl64(state[2], 31) ^ rotl64(state[3], 47);

    for (std::size_t i = 0; i < WORDS; ++i) {
        x += UINT64_C(0x9e3779b97f4a7c15) + static_cast<std::uint64_t>(i);
        x ^= state[i & 3] + rotl64(x, static_cast<unsigned int>((i & 31) + 1));
        x = avalanche(x);
        scratch[i] = x;
        state[i & 3] ^= x + rotl64(state[(i + 1) & 3], 23);
    }

    // Data-dependent random-access phase. v1.1 deliberately uses fewer heavy
    // avalanche operations than v1 and touches three independently derived
    // locations per round. This makes latency and memory traffic a larger
    // fraction of total cost instead of simply burning ALU cycles.
    for (std::size_t round = 0; round < RANDOM_MIX_ROUNDS; ++round) {
        const std::size_t lane = round & 3;

        const std::size_t idx1 = static_cast<std::size_t>(
            state[lane] ^ x ^ static_cast<std::uint64_t>(round)) & (WORDS - 1);
        const std::uint64_t a = scratch[idx1];

        const std::size_t idx2 = static_cast<std::size_t>(
            a ^ rotl64(state[(lane + 1) & 3], 17)) & (WORDS - 1);
        const std::uint64_t b = scratch[idx2];

        const std::size_t idx3 = static_cast<std::size_t>(
            b ^ rotl64(a, 31) ^ state[(lane + 2) & 3]) & (WORDS - 1);
        const std::uint64_t c = scratch[idx3];

        const std::uint64_t mixed = a + rotl64(b, 23) + rotl64(c, 41)
            + state[(lane + 3) & 3] + static_cast<std::uint64_t>(round);

        state[lane] ^= mixed;
        state[lane] = rotl64(state[lane], static_cast<unsigned int>((mixed & 31) + 1));
        state[(lane + 1) & 3] += a ^ rotl64(c, 13);

        scratch[idx1] = a ^ state[lane] ^ rotl64(c, 7);
        scratch[idx2] = b + state[(lane + 1) & 3] + rotl64(a, 19);
        scratch[idx3] = c ^ mixed ^ rotl64(b, 37);
        x = rotl64(x ^ mixed ^ scratch[idx3], 29);
    }

    // Fold the entire scratchpad so all 16 MiB influence the final digest and
    // the memory cannot simply be skipped after the random-access phase.
    std::uint64_t fold0 = state[0];
    std::uint64_t fold1 = state[1];
    std::uint64_t fold2 = state[2];
    std::uint64_t fold3 = state[3];
    for (std::size_t i = 0; i < WORDS; i += 4) {
        fold0 = avalanche(fold0 ^ scratch[i]);
        fold1 = avalanche(fold1 + scratch[i + 1]);
        fold2 = avalanche(fold2 ^ rotl64(scratch[i + 2], 17));
        fold3 = avalanche(fold3 + rotl64(scratch[i + 3], 41));
    }

    std::array<unsigned char, 32> folded{};
    store64le(folded.data(), fold0);
    store64le(folded.data() + 8, fold1);
    store64le(folded.data() + 16, fold2);
    store64le(folded.data() + 24, fold3);

    CSHA256()
        .Write(DOMAIN_FINAL, sizeof(DOMAIN_FINAL))
        .Write(seed.data(), seed.size())
        .Write(folded.data(), folded.size())
        .Write(reinterpret_cast<const unsigned char*>(input), HEADER_SIZE)
        .Finalize(reinterpret_cast<unsigned char*>(output));
}
