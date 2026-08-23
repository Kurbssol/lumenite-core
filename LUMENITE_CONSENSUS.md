# Lumenite Consensus Specification

**Project:** Lumenite  
**Ticker:** LMT  
**Proof-of-Work:** HomeScrypt v1.1  
**Specification:** Testnet v0.3  
**Status:** Testnet consensus freeze  
**Date:** August 22, 2026

---

## 1. Overview

Lumenite is a Proof-of-Work cryptocurrency designed around the
HomeScrypt v1.1 mining algorithm.

The network is intended to support independent CPU and GPU mining while
retaining a Bitcoin-style UTXO monetary system and fixed issuance schedule.

This document records the consensus and monetary parameters used for the
Lumenite Testnet v0.3 release.

Parameters documented as frozen should not be changed during the v0.3
public testnet without intentionally introducing a new consensus version.

LWMA-45 is the active difficulty-adjustment algorithm for Lumenite
Testnet v0.3. Its behavior continues to be evaluated before the final
Lumenite mainnet consensus rules are selected.

---

## 2. Network Identity

Coin name:

    Lumenite

Ticker:

    LMT

Consensus mechanism:

    Proof of Work

Proof-of-Work algorithm:

    HomeScrypt v1.1

Lumenite uses independent mainnet, testnet, and regtest genesis blocks,
network magic values, ports, address prefixes, and chain parameters.

It does not share a blockchain with Litecoin.

---

## 3. HomeScrypt v1.1

HomeScrypt v1.1 is the Proof-of-Work algorithm used by Lumenite.

All valid Lumenite Proof-of-Work blocks must satisfy the HomeScrypt v1.1
consensus implementation included in the corresponding Lumenite Core
source release.

The algorithm has been tested using both CPU and NVIDIA CUDA miners.

During development testing, independently running CPU and CUDA miners
successfully:

- retrieved block templates;
- performed HomeScrypt v1.1 work;
- submitted valid blocks;
- detected stale work;
- followed new chain tips; and
- competed for blocks on the same test network.

The consensus implementation in Lumenite Core is authoritative. External
miners must reproduce its block-header hashing behavior exactly.

---

## 4. Monetary Policy

### Initial block subsidy

The initial block subsidy is:

    50 LMT

### Subsidy halving interval

The subsidy halves every:

    840,000 blocks

At the target block spacing of 150 seconds, this corresponds to
approximately four years.

### Subsidy schedule

The initial subsidy eras are:

| Block Height | Block Subsidy |
|-------------:|--------------:|
| 0 - 839,999 | 50 LMT |
| 840,000 - 1,679,999 | 25 LMT |
| 1,680,000 - 2,519,999 | 12.5 LMT |
| 2,520,000 - 3,359,999 | 6.25 LMT |

Subsequent eras continue halving according to the same rule.

Once the integer subsidy becomes zero, no further block subsidy is
created.

### Supply limit

Lumenite defines:

    MAX_MONEY = 84,000,000 LMT

The subsidy schedule approaches approximately 84 million LMT.

Integer truncation at the smallest monetary unit means actual subsidy
issuance may be slightly below the mathematical 84 million limit.

### Premine

    None

### Founder allocation

    None

### Developer reward

    None

### Treasury reward

    None

Lumenite does not divert a percentage of the block subsidy to a founder,
developer, treasury, organization, or other privileged address.

---

## 5. Coinbase Maturity

Newly mined coinbase outputs require:

    100 blocks

of maturity before they may be spent.

The consensus rule is represented by:

    COINBASE_MATURITY = 100

A coinbase output with fewer than 100 blocks of maturity cannot be spent.

---

## 6. Block Timing

Target block spacing:

    150 seconds

Equivalent to:

    2.5 minutes

Expected blocks per hour:

    24

Expected blocks per day:

    576

Expected blocks per 365-day year:

    210,240

A halving interval of 840,000 blocks therefore corresponds to
approximately four years at target spacing.

---

## 7. Difficulty Adjustment

### Testnet v0.3

Lumenite Testnet v0.3 uses a per-block Linearly Weighted Moving Average
difficulty-adjustment algorithm (LWMA).

Consensus parameters:

    Algorithm:                 LWMA
    Window:                    45 blocks
    Target block spacing:      150 seconds
    Adjustment frequency:      Every block
    LWMA activation height:    46
    Minimum-difficulty blocks: Disabled
    Difficulty retargeting:    Enabled

The LWMA algorithm evaluates the most recent 45 solve-time samples and
weights newer solve times more heavily than older solve times.

A complete LWMA window requires 45 solve-time samples, corresponding to
46 block timestamps.

Individual solve times are bounded to:

    Minimum solve time:        1 second
    Maximum solve time:        900 seconds

The maximum solve time is six times the target block spacing:

    6 * 150 = 900 seconds

This prevents a single extremely delayed block from having unlimited
influence over the next difficulty calculation.

The algorithm also limits difficulty movement from one block to the next.

In target space, the permitted range is approximately:

    Hardest next target:       previous target * 4 / 5
    Easiest next target:       previous target * 4 / 3

Because target and difficulty are inversely related, this corresponds to
approximately a maximum 25 percent difficulty increase or decrease per
block.

The calculated target may never exceed the network Proof-of-Work limit.

### LWMA activation

LWMA activates for the block whose height is:

    46

The next block height is evaluated before selecting the difficulty
algorithm.

Therefore:

    Blocks before height 46:   pre-LWMA difficulty behavior
    Block 46 and later:        LWMA-45

The activation height is consensus-critical. Nodes participating on the
same Testnet v0.3 chain must use identical activation rules.

### Mainnet

Mainnet currently retains the legacy windowed difficulty-adjustment
algorithm.

Minimum-difficulty blocks:

    Disabled

Difficulty retargeting:

    Enabled

The Testnet v0.3 LWMA deployment does not by itself activate LWMA on
Lumenite mainnet.

### Testnet

Minimum-difficulty blocks:

    Disabled

Difficulty retargeting:

    Enabled

Difficulty algorithm:

    LWMA-45

Unlike the earlier testnet configuration, the Litecoin-style
minimum-difficulty shortcut is disabled while LWMA is active.

### Regtest

Automatic difficulty retargeting:

    Disabled

Regtest parameters exist for development and automated testing and are
not part of Lumenite's production monetary policy.

---

## 8. Difficulty Algorithm Status

LWMA-45 is the active difficulty-adjustment algorithm for Lumenite
Testnet v0.3.

The algorithm recalculates difficulty every block rather than waiting for
a fixed multi-day retarget interval.

Development testing prior to the Testnet v0.3 specification evaluated
multiple candidate LWMA history windows, including:

- LWMA-30;
- LWMA-45; and
- LWMA-60.

LWMA-45 was selected for Testnet v0.3 as a balance between responsiveness
to changing network hashrate and resistance to excessive short-term
difficulty variance.

Testing included scenarios involving:

- rapid GPU hashrate increases;
- rapid GPU hashrate decreases;
- CPU-only mining;
- transitions between GPU and CPU mining;
- miners joining and leaving;
- large temporary hashrate increases;
- major hashrate loss;
- short hashrate bursts;
- hash-hopping simulations;
- low total network hashrate; and
- random block-time variance.

CPU and NVIDIA CUDA mining have both successfully produced valid blocks
while the LWMA difficulty changed dynamically.

The Testnet v0.3 LWMA configuration is:

    Window:                    45 blocks
    Target spacing:            150 seconds
    Adjustment frequency:      Every block
    Activation height:         46
    Minimum-difficulty rule:   Disabled

LWMA-45 is frozen for the Testnet v0.3 consensus rules so that all
Testnet v0.3 nodes remain consensus-compatible.

The final mainnet difficulty-adjustment configuration remains subject to
testing, review, and an explicit mainnet consensus freeze.

---

## 9. Consensus Feature Activation

Lumenite is a new blockchain and therefore does not reproduce historical
Litecoin activation timelines.

The Testnet v0.3 mainnet/testnet consensus configuration uses:

| Consensus Feature | Activation |
|---|---:|
| BIP16 | Genesis |
| BIP34 | Height 1 |
| BIP65 | Height 1 |
| BIP66 | Height 1 |
| CSV | Height 1 |
| SegWit | Height 1 |
| Taproot | Genesis / always active |
| MWEB | Disabled |

These activation heights establish modern transaction and script rules
near the beginning of the Lumenite blockchain instead of replaying
historical activation schedules inherited from another network.

---

## 10. MWEB

MimbleWimble Extension Blocks (MWEB) are:

    Disabled

MWEB is not part of the Lumenite Testnet v0.3 consensus specification.

No assumption should be made that MWEB will be activated in a future
Lumenite release.

Adding MWEB or another consensus extension in the future would require a
separate design, implementation, testing, and activation process.

---

## 11. Genesis Blocks

Lumenite uses independently generated genesis blocks for:

- Mainnet
- Testnet
- Regtest

The genesis blocks were generated specifically for Lumenite and its
HomeScrypt v1.1 Proof-of-Work implementation.

### Mainnet Genesis

Genesis block hash:

    0611e3203edbfad2296bd691e76e5e77d7d383a00f7e99d313802db700a1fbf1

Merkle root:

    c9405c101b7c92ca39f74c7564d46ebb399aa480e76c55b7b6fa0d63963ebc95

### Testnet Genesis

The authoritative Testnet v0.3 genesis hash is the value compiled into
the finalized Lumenite Core `chainparams.cpp`.

Testnet nodes MUST agree on this genesis block before participating in
the same test network.

### Regtest Genesis

Genesis block hash:

    f112881b7e0d693d7eee2c0f9a08020c89fad4f40d0d51cefad588207a6f7293

Merkle root:

    c9405c101b7c92ca39f74c7564d46ebb399aa480e76c55b7b6fa0d63963ebc95

The source code remains authoritative for all genesis parameters,
including timestamp, nonce, bits, version, reward, and hash.

---

## 12. Testnet v0.3 Addressing

Lumenite Testnet uses its own address configuration.

Native SegWit testnet addresses use the Lumenite testnet HRP:

    tlmt

Example form:

    tlmt1...

Mainnet and testnet addresses must not be treated as interchangeable.

The authoritative Base58 prefixes and Bech32 HRPs are defined by the
corresponding Lumenite Core chain parameters.

---

## 13. Consensus Verification

The Lumenite validation test suite
explicitly verified the monetary-policy boundaries.

Expected subsidy behavior includes:

    Height 0          = 50 LMT
    Height 1          = 50 LMT
    Height 839,999    = 50 LMT

    Height 840,000    = 25 LMT
    Height 1,679,999  = 25 LMT

    Height 1,680,000  = 12.5 LMT
    Height 2,519,999  = 12.5 LMT

    Height 2,520,000  = 6.25 LMT

The test suite also verifies:

    nSubsidyHalvingInterval = 840,000
    MAX_MONEY               = 84,000,000 LMT
    COINBASE_MATURITY       = 100

Generic subsidy-halving tests verify that subsidy continues halving and
eventually reaches zero.

The existing block-validation test infrastructure exercises coinbase
maturity behavior.

The validation test suite passed before this specification was frozen.

---

## 14. Regtest

Regtest is intentionally allowed to differ from mainnet and testnet.

Values such as:

- shorter subsidy-halving intervals;
- different activation heights;
- immediately active features; and
- disabled difficulty retargeting

may be used to support automated tests and local development.

Regtest parameters MUST NOT be interpreted as Lumenite mainnet monetary
policy.

---

## 15. Testnet v0.3 Goals

The public Lumenite Testnet v0.3 is intended to validate:

1. HomeScrypt v1.1 consensus compatibility across hardware.
2. CPU mining.
3. NVIDIA CUDA mining.
4. Multi-node synchronization.
5. Block propagation.
6. Chain reorganizations.
7. Stale-work handling.
8. Wallet operation.
9. Transaction propagation.
10. Coinbase maturity.
11. Long-running node stability.
12. Difficulty-adjustment behavior.
13. Hashrate changes.
14. Miner interoperability.
15. Windows and Linux client compatibility.

Testnet coins have no monetary value and are intended solely for
development and testing.

---

## 16. Consensus Freeze Policy

The following parameters are frozen for Lumenite Testnet v0.3:

    Proof of Work:           HomeScrypt v1.1
    Initial subsidy:         50 LMT
    Halving interval:        840,000 blocks
    MAX_MONEY:               84,000,000 LMT
    Coinbase maturity:       100 blocks
    Target block spacing:    150 seconds
    Testnet DAA:             LWMA-45
    LWMA activation height:  46
    LWMA adjustment:         every block
    Testnet min difficulty:  disabled
    MWEB:                    disabled

Consensus feature activation parameters documented in this specification
are also part of the Testnet v0.3 configuration.

LWMA-45 is frozen for the lifetime of Testnet v0.3 so all v0.3 nodes
remain consensus-compatible.

This freeze applies to Testnet v0.3 only. The final mainnet
difficulty-adjustment configuration remains subject to review.

Changing a consensus-critical parameter during public testing should
result in a new testnet version rather than silently changing the
existing Testnet v0.3 rules.

---

## 17. Source Code Authority

This document describes the intended Lumenite Testnet v0.3 consensus
rules.

In the event of a discrepancy between this document and the executable
consensus implementation, the behavior enforced by the corresponding
Lumenite Core source release determines blockchain validity.

Any discrepancy should be treated as a bug and resolved before a
mainnet release.

---

## 18. Mainnet Readiness

Lumenite Testnet v0.3 does not constitute a mainnet launch.

Before mainnet, the project should complete:

- extended public testnet operation;
- CPU and GPU hardware testing;
- Windows and Linux testing;
- multi-node synchronization testing;
- network partition/reorganization testing;
- transaction and wallet testing;
- continued LWMA-45 difficulty-adjustment analysis;
- ASIC-resistance and specialized-hardware testing;
- code review;
- final network parameter review;
- final mainnet genesis verification; and
- final consensus freeze.

The mainnet difficulty-adjustment algorithm and its final parameters must
be explicitly selected and documented before the final Lumenite mainnet
consensus freeze.

---

## Lumenite Testnet v0.3

**Coin:** Lumenite  
**Ticker:** LMT  
**PoW:** HomeScrypt v1.1  
**Initial Subsidy:** 50 LMT  
**Halving Interval:** 840,000 blocks  
**Supply Limit:** approximately 84 million LMT  
**Block Target:** 150 seconds  
**Coinbase Maturity:** 100 blocks  
**MWEB:** Disabled  
**Status:** Public Testnet Candidate
