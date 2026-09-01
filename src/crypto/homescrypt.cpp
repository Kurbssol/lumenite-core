// Experimental HomeScrypt v1.1 proof-of-work for the HomeLite development fork.
// NOT frozen consensus. This implementation is deliberately simple enough to
// audit and port before CPU/GPU optimization work begins.

#include <crypto/homescrypt.h>
#include <crypto/scrypt.h>
#include <crypto/sha256.h>

#include <array>
#include <cmath>
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

// v1.2. Only the product of these matters: it sets the FP32 FMAs per hash,
// mix_rounds * fp_passes * FP_INNER * 4, here ~2.7e8. Rounds come down as FP
// work goes up; v1.1's memory cost is too dominant otherwise. 128 passes puts
// a hash at 238ms against v1.1's 55ms, measured. See doc/homescrypt-v12.md.
constexpr std::size_t RANDOM_MIX_ROUNDS_V12 = WORDS / 32;
constexpr std::size_t FP_PASSES_V12 = 128;

constexpr unsigned char DOMAIN_SEED_V12[] = {
    'H','O','M','E','S','C','R','Y','P','T','-','V','1','.','2','-','S','E','E','D'
};
constexpr unsigned char DOMAIN_FINAL_V12[] = {
    'H','O','M','E','S','C','R','Y','P','T','-','V','1','.','2','-','F','I','N','A','L'
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

// HomeScrypt v1.2. Same scratchpad, fill and fold as v1.1, so memory-hardness
// is unchanged. The mix phase trades 16x fewer rounds for a dense IEEE-754
// binary32 FMA chain whose result feeds state[lane] and x, gating the next
// address. This moves cost off memory latency, where an FPGA with HBM competes
// with a GPU on even terms, and onto FP32 throughput, where it does not.
//
// Callers must build with FP contraction disabled and fast-math off, or the
// digest differs between implementations. See doc/homescrypt-v12.md.

namespace {

inline std::uint32_t f32_bits(float f)
{
    std::uint32_t w;
    std::memcpy(&w, &f, sizeof(w));
    return w;
}

inline float bits_f32(std::uint32_t w)
{
    float f;
    std::memcpy(&f, &w, sizeof(f));
    return f;
}

// Any word to a positive normal float in [2^-9, 2^7): never NaN, infinite or
// denormal, which are the three cases where CPU, CUDA and OpenCL are allowed
// to disagree. The four exponent bits are load-bearing -- a constant exponent
// would reduce every FMA to fixed point, which is cheap in fabric.
inline float safe_f32(std::uint32_t w)
{
    const std::uint32_t exp = 118u + ((w >> 23) & 0x0Fu);
    return bits_f32((exp << 23) | (w & 0x007FFFFFu));
}

// FMAs between renormalisations, as 4-wide iterations. Worst case peaks at
// 2^70.6 against binary32's 2^128 and first overflows at 16, so this has 2x
// margin. Do not raise it without re-measuring: an infinity here splits the
// chain rather than rounding differently.
constexpr int FP_INNER = 8;

} // namespace

void homescrypt_v12_tuned(const char* input, char* output,
                          std::size_t mix_rounds, std::size_t fp_passes)
{
    std::array<unsigned char, 32> scrypt_seed{};
    scrypt_1024_1_1_256(input, reinterpret_cast<char*>(scrypt_seed.data()));

    std::array<unsigned char, 32> seed{};
    CSHA256()
        .Write(DOMAIN_SEED_V12, sizeof(DOMAIN_SEED_V12))
        .Write(reinterpret_cast<const unsigned char*>(input), HEADER_SIZE)
        .Write(scrypt_seed.data(), scrypt_seed.size())
        .Finalize(seed.data());

    std::array<std::uint64_t, 4> state{{
        load64le(seed.data()),
        load64le(seed.data() + 8),
        load64le(seed.data() + 16),
        load64le(seed.data() + 24)
    }};

    // Fill: unchanged from v1.1.
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

    for (std::size_t round = 0; round < mix_rounds; ++round) {
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

        // Four chains rather than one: a single chain would let one pipelined
        // FMA unit serve many hash instances, and four let a single nonce keep
        // a CPU's FMA pipeline busy.
        float m0 = safe_f32(static_cast<std::uint32_t>(c >> 32));
        float m1 = safe_f32(static_cast<std::uint32_t>(c));
        float m2 = safe_f32(static_cast<std::uint32_t>(mixed >> 32));
        float m3 = safe_f32(static_cast<std::uint32_t>(mixed));

        float g0 = safe_f32(static_cast<std::uint32_t>(a >> 32));
        float g1 = safe_f32(static_cast<std::uint32_t>(a));
        float g2 = safe_f32(static_cast<std::uint32_t>(b >> 32));
        float g3 = safe_f32(static_cast<std::uint32_t>(b));

        for (std::size_t pass = 0; pass < fp_passes; ++pass) {
            for (int j = 0; j < FP_INNER; ++j) {
                // fmaf, never g * m + h: the fused form rounds once. A
                // compiler that contracts on one platform and not another
                // gives a different digest.
                g0 = std::fmaf(g0, m0, g3);
                g1 = std::fmaf(g1, m1, g0);
                g2 = std::fmaf(g2, m2, g1);
                g3 = std::fmaf(g3, m3, g2);
            }

            const std::uint32_t w0 = f32_bits(g0);
            const std::uint32_t w1 = f32_bits(g1);
            const std::uint32_t w2 = f32_bits(g2);
            const std::uint32_t w3 = f32_bits(g3);

            g0 = safe_f32(w0);
            g1 = safe_f32(w1);
            g2 = safe_f32(w2);
            g3 = safe_f32(w3);

            // Rotate the multipliers in, so the inner loop is not one fixed
            // recurrence repeated fp_passes times.
            const float t = m0;
            m0 = safe_f32(f32_bits(m1) ^ w0);
            m1 = safe_f32(f32_bits(m2) ^ w1);
            m2 = safe_f32(f32_bits(m3) ^ w2);
            m3 = safe_f32(f32_bits(t) ^ w3);
        }

        const std::uint64_t fp =
            ((static_cast<std::uint64_t>(f32_bits(g0)) << 32) | f32_bits(g1))
            ^ rotl64((static_cast<std::uint64_t>(f32_bits(g2)) << 32) | f32_bits(g3), 29);

        // fp reaches both state[lane] and x, and the next round opens with
        // (state[lane] ^ x ^ round). Without this the FP work would still be
        // mandatory but could overlap with memory instead of gating it.
        state[lane] ^= mixed ^ fp;
        state[lane] = rotl64(state[lane], static_cast<unsigned int>((mixed & 31) + 1));
        state[(lane + 1) & 3] += a ^ rotl64(c, 13);

        scratch[idx1] = a ^ state[lane] ^ rotl64(c, 7);
        scratch[idx2] = b + state[(lane + 1) & 3] + rotl64(a, 19);
        scratch[idx3] = c ^ mixed ^ rotl64(b, 37);
        x = rotl64(x ^ mixed ^ fp ^ scratch[idx3], 29);
    }

    // Fold: unchanged from v1.1. With fewer mix rounds this is now the main
    // reason the full 16 MiB cannot be skipped.
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
        .Write(DOMAIN_FINAL_V12, sizeof(DOMAIN_FINAL_V12))
        .Write(seed.data(), seed.size())
        .Write(folded.data(), folded.size())
        .Write(reinterpret_cast<const unsigned char*>(input), HEADER_SIZE)
        .Finalize(reinterpret_cast<unsigned char*>(output));
}

void homescrypt_v12(const char* input, char* output)
{
    homescrypt_v12_tuned(input, output, RANDOM_MIX_ROUNDS_V12, FP_PASSES_V12);
}

float homescrypt_v12_safe_f32(std::uint32_t w)
{
    return safe_f32(w);
}
