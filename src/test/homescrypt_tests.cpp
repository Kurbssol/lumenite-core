#include <boost/test/unit_test.hpp>

#include <crypto/homescrypt.h>
#include <uint256.h>

#include <array>

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

BOOST_AUTO_TEST_SUITE_END()
