// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>


/**
 * Lumenite LWMA per-block difficulty adjustment.
 *
 * This intentionally mirrors the behavior tested by the Lumenite
 * Python DAA simulations:
 *
 *   - recalculates difficulty every block
 *   - linearly weights newer solve times more heavily
 *   - caps individual solve times to [1, 6*T]
 *   - averages the target over the same history window
 *   - limits target movement to +/-25% per block
 *
 * NOTE:
 * This implementation is enabled according to the active network consensus parameters.
 */
unsigned int GetNextWorkRequiredLWMA(
    const CBlockIndex* pindexLast,
    const Consensus::Params& params)
{
    assert(pindexLast != nullptr);

    const arith_uint256 pow_limit = UintToArith256(params.powLimit);
    const unsigned int pow_limit_compact = pow_limit.GetCompact();

    if (params.fPowNoRetargeting) {
        return pindexLast->nBits;
    }

    const int N = params.nPowLWMAWindow;
    const int64_t T = params.nPowTargetSpacing;

    if (N <= 0 || T <= 0) {
        return pow_limit_compact;
    }

    /*
     * Not enough history yet.
     *
     * Keep using the previous block's target until N complete
     * solve-time samples are available.
     */
    if (pindexLast->nHeight < N) {
        return pindexLast->nBits;
    }

    /*
     * We need N solve times, which requires N+1 block timestamps.
     *
     * pindexLast is block H.
     * Start with block H-N as the previous timestamp and process:
     *
     *     H-N+1 ... H
     */
    const CBlockIndex* first = pindexLast->GetAncestor(
        pindexLast->nHeight - N
    );

    if (first == nullptr) {
        return pindexLast->nBits;
    }

    int64_t previous_time = first->GetBlockTime();

    /*
     * Weighted solve-time accumulator.
     *
     * Maximum:
     *   6*T * N*(N+1)/2
     *
     * which is comfortably inside int64_t for our parameters.
     */
    int64_t weighted_solve_sum = 0;
    int64_t weight_sum = 0;

    /*
     * Average target over the N-block history.
     *
     * Divide each target before summing to avoid uint256 overflow.
     */
    arith_uint256 average_target;
    average_target = 0;

    for (int i = 1; i <= N; ++i) {
        const int height =
            pindexLast->nHeight - N + i;

        const CBlockIndex* block =
            pindexLast->GetAncestor(height);

        if (block == nullptr) {
            return pindexLast->nBits;
        }

        int64_t current_time = block->GetBlockTime();

        /*
         * Protect against equal/backward timestamps.
         */
        if (current_time <= previous_time) {
            current_time = previous_time + 1;
        }

        int64_t solve_time =
            current_time - previous_time;

        /*
         * Bound individual solve times to:
         *
         * minimum 1 second
         * maximum 6 target spacings
         */
        if (solve_time < 1) {
            solve_time = 1;
        }

        if (solve_time > 6 * T) {
            solve_time = 6 * T;
        }

        weighted_solve_sum += solve_time * i;
        weight_sum += i;

        previous_time = current_time;

        arith_uint256 target;
        target.SetCompact(block->nBits);

        average_target += target / N;
    }

    if (weighted_solve_sum <= 0 ||
        weight_sum <= 0 ||
        average_target == 0) {
        return pindexLast->nBits;
    }

    /*
     * Weighted average solve time.
     */
    const int64_t weighted_solve =
        weighted_solve_sum / weight_sum;

    if (weighted_solve <= 0) {
        return pindexLast->nBits;
    }

    /*
     * Candidate target:
     *
     *     avg_target * weighted_solve / T
     *
     * Faster-than-target blocks => smaller target => harder.
     * Slower-than-target blocks => larger target => easier.
     *
     * arith_uint256 is fixed-width, so avoid multiplying a near-powLimit
     * target directly by a value as large as 6*T.
     *
     * Divide first when making the target easier. When making the target
     * harder, multiplication is bounded by weighted_solve <= T and the
     * result cannot exceed the starting target.
     */
    arith_uint256 next_target = average_target;

    if (weighted_solve >= T) {
        next_target /= T;
        next_target *= weighted_solve;
    } else {
        next_target *= weighted_solve;
        next_target /= T;
    }

    /*
     * Per-block difficulty movement limiter.
     *
     * TARGET is inverse of difficulty:
     *
     * target * 0.80  ~= difficulty +25%
     * target * 4/3   ~= difficulty -25%
     *
     * The equivalent difficulty bounds are:
     *
     *   previous * 0.75 .. previous * 1.25
     *
     * in inverse-target space.
     */
    arith_uint256 previous_target;
    previous_target.SetCompact(pindexLast->nBits);

    arith_uint256 hardest_allowed = previous_target;
    hardest_allowed *= 4;
    hardest_allowed /= 5; // target -20% => difficulty +25%

    arith_uint256 easiest_allowed = previous_target;
    easiest_allowed *= 4;
    easiest_allowed /= 3; // target +33.3% => difficulty -25%

    if (next_target < hardest_allowed) {
        next_target = hardest_allowed;
    }

    if (next_target > easiest_allowed) {
        next_target = easiest_allowed;
    }

    if (next_target > pow_limit) {
        next_target = pow_limit;
    }

    if (next_target == 0) {
        return pow_limit_compact;
    }

    return next_target.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    /*
     * Lumenite per-block LWMA path.
     *
     * The next block height is pindexLast->nHeight + 1.
     */
    const int64_t nextHeight = pindexLast->nHeight + 1;

    if (params.fPowUseLWMA &&
        nextHeight >= params.nPowLWMAActivationHeight) {
        return GetNextWorkRequiredLWMA(pindexLast, params);
    }

    // Legacy windowed difficulty adjustment.
    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    // Litecoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = params.DifficultyAdjustmentInterval()-1;
    if ((pindexLast->nHeight+1) != params.DifficultyAdjustmentInterval())
        blockstogoback = params.DifficultyAdjustmentInterval();

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;

    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    // Litecoin: intermediate uint256 can overflow by 1 bit
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    bool fShift = bnNew.bits() > bnPowLimit.bits() - 1;
    if (fShift)
        bnNew >>= 1;
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;
    if (fShift)
        bnNew <<= 1;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
