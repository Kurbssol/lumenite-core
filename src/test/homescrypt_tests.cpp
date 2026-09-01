#include <boost/test/unit_test.hpp>

#include <crypto/homescrypt.h>
#include <uint256.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

BOOST_AUTO_TEST_SUITE(homescrypt_tests)

BOOST_AUTO_TEST_CASE(deterministic)
{
    std::array<unsigned char, 80> header{};
    uint256 hash0;
    uint256 hash1;

    homescrypt_v11(
        reinterpret_cast<const char*>(header.data()),
        reinterpret_cast<char*>(hash0.data())
    );

    homescrypt_v11(
        reinterpret_cast<const char*>(header.data()),
        reinterpret_cast<char*>(hash1.data())
    );

    BOOST_CHECK(hash0 == hash1);

    BOOST_CHECK_EQUAL(
        hash0.ToString(),
        "9d795788e43a9cf09cc2119c44006b0750f654b16f7cf5395a2b2ba9fee4c433"
    );
}

BOOST_AUTO_TEST_CASE(nonce_changes_pow)
{
    std::array<unsigned char, 80> header0{};
    std::array<unsigned char, 80> header1{};

    header1[76] = 1;

    uint256 hash0;
    uint256 hash1;

    homescrypt_v11(
        reinterpret_cast<const char*>(header0.data()),
        reinterpret_cast<char*>(hash0.data())
    );

    homescrypt_v11(
        reinterpret_cast<const char*>(header1.data()),
        reinterpret_cast<char*>(hash1.data())
    );

    BOOST_CHECK(hash0 != hash1);
}

BOOST_AUTO_TEST_CASE(scratchpad_size)
{
    BOOST_CHECK_EQUAL(
        homescrypt_v11_scratchpad_bytes(),
        16U * 1024U * 1024U
    );
}

BOOST_AUTO_TEST_CASE(v12_deterministic)
{
    std::array<unsigned char, 80> header{};
    uint256 hash0;
    uint256 hash1;

    homescrypt_v12(
        reinterpret_cast<const char*>(header.data()),
        reinterpret_cast<char*>(hash0.data())
    );

    homescrypt_v12(
        reinterpret_cast<const char*>(header.data()),
        reinterpret_cast<char*>(hash1.data())
    );

    BOOST_CHECK(hash0 == hash1);

    // Not pinned yet: a vector committed before the CUDA and OpenCL ports
    // agree would only pin what this compiler happened to do.
    BOOST_TEST_MESSAGE("HomeScrypt v1.2, zero header: " << hash0.ToString());
}

BOOST_AUTO_TEST_CASE(v12_differs_from_v11)
{
    std::array<unsigned char, 80> header{};
    uint256 v11;
    uint256 v12;

    homescrypt_v11(
        reinterpret_cast<const char*>(header.data()),
        reinterpret_cast<char*>(v11.data())
    );
    homescrypt_v12(
        reinterpret_cast<const char*>(header.data()),
        reinterpret_cast<char*>(v12.data())
    );

    BOOST_CHECK(v11 != v12);

    // v1.1 must be untouched by this change.
    BOOST_CHECK_EQUAL(
        v11.ToString(),
        "9d795788e43a9cf09cc2119c44006b0750f654b16f7cf5395a2b2ba9fee4c433"
    );
}

BOOST_AUTO_TEST_CASE(v12_nonce_changes_pow)
{
    std::array<unsigned char, 80> header0{};
    std::array<unsigned char, 80> header1{};

    header1[76] = 1;

    uint256 hash0;
    uint256 hash1;

    homescrypt_v12(
        reinterpret_cast<const char*>(header0.data()),
        reinterpret_cast<char*>(hash0.data())
    );
    homescrypt_v12(
        reinterpret_cast<const char*>(header1.data()),
        reinterpret_cast<char*>(hash1.data())
    );

    BOOST_CHECK(hash0 != hash1);
}

// NaN, infinities and denormals are where CPU, CUDA and OpenCL may disagree.
// v1.2 avoids them by construction; this asserts the construction holds.
BOOST_AUTO_TEST_CASE(v12_safe_f32_never_degenerate)
{
    const float lo = std::ldexp(1.0f, -9);   // 2^-9
    const float hi = std::ldexp(1.0f, 7);    // 2^7

    // Every value of the exponent nibble, at both mantissa extremes, plus the
    // all-zero and all-ones words.
    std::vector<std::uint32_t> probes{0x00000000u, 0xFFFFFFFFu};
    for (std::uint32_t nib = 0; nib < 16; ++nib) {
        probes.push_back(nib << 23);
        probes.push_back((nib << 23) | 0x007FFFFFu);
        probes.push_back(0x80000000u | (nib << 23));
    }

    // Plus a deterministic spread across the space.
    std::uint64_t rng = 0x243F6A8885A308D3ull;
    for (int i = 0; i < 200000; ++i) {
        rng ^= rng << 13;
        rng ^= rng >> 7;
        rng ^= rng << 17;
        probes.push_back(static_cast<std::uint32_t>(rng));
    }

    for (const std::uint32_t w : probes) {
        const float f = homescrypt_v12_safe_f32(w);
        BOOST_REQUIRE_MESSAGE(std::isnormal(f),
            "safe_f32(" << w << ") is not a normal float");
        BOOST_REQUIRE_MESSAGE(f >= lo && f < hi,
            "safe_f32(" << w << ") = " << f << " outside [2^-9, 2^7)");
    }
}

// The chain must not reach infinity: overflow would split the chain rather
// than round differently. Worst case is 2^70.6 against binary32's 2^128.
BOOST_AUTO_TEST_CASE(v12_fma_chain_cannot_overflow)
{
    // The largest value safe_f32 can produce: exponent nibble 15, full mantissa.
    const float m = homescrypt_v12_safe_f32(0x07FFFFFFu);
    float g0 = m, g1 = m, g2 = m, g3 = m;

    for (int j = 0; j < 8; ++j) {
        g0 = std::fmaf(g0, m, g3);
        g1 = std::fmaf(g1, m, g0);
        g2 = std::fmaf(g2, m, g1);
        g3 = std::fmaf(g3, m, g2);
    }

    BOOST_CHECK(std::isfinite(g0) && std::isfinite(g1));
    BOOST_CHECK(std::isfinite(g2) && std::isfinite(g3));
    BOOST_CHECK_LT(std::log2(g3), 80.0);
}

BOOST_AUTO_TEST_CASE(v12_tuning_affects_digest)
{
    std::array<unsigned char, 80> header{};
    uint256 slow;
    uint256 fast;

    // Small parameters to stay cheap; the point is that the knob reaches the
    // digest, so the benchmark sweep measures distinct work.
    homescrypt_v12_tuned(
        reinterpret_cast<const char*>(header.data()),
        reinterpret_cast<char*>(slow.data()), 64, 4);
    homescrypt_v12_tuned(
        reinterpret_cast<const char*>(header.data()),
        reinterpret_cast<char*>(fast.data()), 64, 8);

    BOOST_CHECK(slow != fast);
}

BOOST_AUTO_TEST_SUITE_END()
