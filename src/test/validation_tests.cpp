// Copyright (c) 2014-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <chainparams.h>
#include <consensus/consensus.h>
#include <net.h>
#include <signet.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_tests, TestingSetup)

static void TestBlockSubsidyHalvings(const Consensus::Params& consensusParams)
{
    const int maxHalvings = 64;
    const CAmount nInitialSubsidy = 50 * COIN;

    CAmount nPreviousSubsidy = nInitialSubsidy * 2; // for height == 0
    BOOST_CHECK_EQUAL(nPreviousSubsidy, nInitialSubsidy * 2);

    for (int nHalvings = 0; nHalvings < maxHalvings; nHalvings++) {
        const int nHeight =
            nHalvings * consensusParams.nSubsidyHalvingInterval;

        const CAmount nSubsidy =
            GetBlockSubsidy(nHeight, consensusParams);

        BOOST_CHECK(nSubsidy <= nInitialSubsidy);
        BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);

        nPreviousSubsidy = nSubsidy;
    }

    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(
            maxHalvings * consensusParams.nSubsidyHalvingInterval,
            consensusParams),
        0);
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
{
    Consensus::Params consensusParams;
    consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;

    TestBlockSubsidyHalvings(consensusParams);
}

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams =
        CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

    // Lumenite mainnet policy.
    TestBlockSubsidyHalvings(chainParams->GetConsensus());

    // Regtest / generic interval coverage.
    TestBlockSubsidyHalvings(150);
    TestBlockSubsidyHalvings(1000);
}

BOOST_AUTO_TEST_CASE(lumenite_subsidy_schedule_test)
{
    const auto chainParams =
        CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

    const Consensus::Params& consensus =
        chainParams->GetConsensus();

    // Lumenite monetary policy.
    BOOST_CHECK_EQUAL(
        consensus.nSubsidyHalvingInterval,
        840000);

    // Initial subsidy era.
    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(0, consensus),
        50 * COIN);

    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(1, consensus),
        50 * COIN);

    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(839999, consensus),
        50 * COIN);

    // First halving.
    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(840000, consensus),
        25 * COIN);

    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(1679999, consensus),
        25 * COIN);

    // Second halving: 12.5 LMT.
    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(1680000, consensus),
        CAmount{1250000000});

    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(2519999, consensus),
        CAmount{1250000000});

    // Third halving: 6.25 LMT.
    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(2520000, consensus),
        CAmount{625000000});

    // Supply ceiling used by MoneyRange().
    BOOST_CHECK_EQUAL(
        MAX_MONEY,
        84000000 * COIN);

    // Lumenite coinbase outputs require 100 confirmations
    // before they may be spent.
    BOOST_CHECK_EQUAL(
        COINBASE_MATURITY,
        100);
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const auto chainParams =
        CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

    CAmount nSum = 0;

    for (int nHeight = 0;
         nHeight < 56000000;
         nHeight += 1000) {

        const CAmount nSubsidy =
            GetBlockSubsidy(
                nHeight,
                chainParams->GetConsensus());

        BOOST_CHECK(nSubsidy <= 50 * COIN);

        nSum += nSubsidy * 1000;

        BOOST_CHECK(MoneyRange(nSum));
    }

    BOOST_CHECK_EQUAL(
        nSum,
        CAmount{8399999990760000});
}

BOOST_AUTO_TEST_SUITE_END()