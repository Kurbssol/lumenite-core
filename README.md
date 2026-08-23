# Lumenite Core

**Lumenite (LMT)** is a Proof-of-Work cryptocurrency built around the **HomeScrypt v1.1** mining algorithm.

The project is currently in **public testnet development**.

HomeScrypt is designed for CPU and GPU mining, with the goal of making mining accessible to home users while maintaining a Bitcoin-style UTXO blockchain and fixed monetary policy.

---

## Project Status

**Current release: Lumenite Core v0.3.0**

**Network status: Public Testnet**

Mainnet has **not launched**.

Lumenite Testnet v0.3 is being used to validate:

- HomeScrypt v1.1 consensus
- LWMA-45 per-block difficulty adjustment
- CPU mining
- NVIDIA CUDA mining
- ASIC-resistance behavior
- block propagation
- multi-node synchronization
- chain reorganizations
- stale-work handling
- wallet operation
- transaction propagation
- Windows and Linux compatibility
- mining performance across different hardware

Testnet coins have no monetary value.

Consensus and network parameters may change before mainnet.

---

## Coin Information

| Parameter | Value |
|---|---|
| Name | Lumenite |
| Ticker | LMT |
| Consensus | Proof of Work |
| Mining Algorithm | HomeScrypt v1.1 |
| Initial Block Reward | 50 LMT |
| Halving Interval | 840,000 blocks |
| Supply Limit | Approximately 84,000,000 LMT |
| Target Block Time | 150 seconds |
| Difficulty Adjustment | LWMA-45 on Testnet v0.3 |
| Difficulty Adjustment Frequency | Every block |
| Coinbase Maturity | 100 blocks |
| Premine | None |
| Developer Reward | None |
| MWEB | Disabled |

See `LUMENITE_CONSENSUS.md` for the full consensus specification.

---

## Difficulty Adjustment

Lumenite Testnet v0.3 introduces a **per-block LWMA-45 difficulty adjustment algorithm**.

Instead of waiting for a large fixed retarget period to complete, difficulty is recalculated for every block using recent block history.

Current Testnet v0.3 parameters:

- **Target block spacing:** 150 seconds
- **LWMA window:** 45 blocks
- **Adjustment frequency:** Every block
- **Testnet activation height:** 46
- **Minimum-difficulty shortcut:** Disabled
- **Per-block movement limiting:** Enabled

The moving window gives greater weight to newer solve times, allowing difficulty to respond to changes in network hashrate considerably faster than the previous long-window retarget system.

LWMA-45 was selected for Testnet v0.3 after simulation and live mining tests involving changing hashrate conditions.

The final mainnet difficulty-adjustment parameters remain subject to testnet validation.

---

## HomeScrypt v1.1

HomeScrypt v1.1 is the Proof-of-Work algorithm used by Lumenite.

It is designed for general-purpose CPU and GPU mining and is intended to make mining practical for home users.

Development and testnet testing have successfully demonstrated:

- CPU mining
- NVIDIA CUDA mining
- independent CPU and GPU miners competing for blocks
- valid block construction and submission
- stale-work detection
- temporary chain forks
- automatic chain reorganization
- multi-node synchronization
- operation with the LWMA-45 difficulty adjustment algorithm

The Lumenite Core consensus implementation is authoritative for block validity.

---

## ASIC Resistance Testing

HomeScrypt v1.1 is intended to favor general-purpose CPU and GPU hardware rather than conventional Scrypt ASIC hardware.

CPU and NVIDIA CUDA mining have been validated on the current testnet.

Direct testing against conventional Scrypt ASIC hardware is part of the Testnet v0.3 validation process.

ASIC resistance should therefore be considered **under active testing** until this validation is complete.

---

## Lumenite Core Binaries

The Lumenite Core source builds the following primary applications:

```text
lumenited
lumenite-cli
lumenite-wallet
lumenite-tx
lumenite-qt
lumenited

Full Lumenite node daemon.

lumenite-cli

Command-line RPC interface for communicating with a running Lumenite node.

lumenite-wallet

Command-line wallet utility.

lumenite-tx

Raw transaction utility.

lumenite-qt

Graphical Lumenite Core wallet and full node.

Testnet

Lumenite is currently operating in testnet development.

To run the testnet daemon:

lumenited -testnet

To communicate with a running testnet node:

lumenite-cli -testnet getblockchaininfo

Example commands:

lumenite-cli -testnet getblockcount
lumenite-cli -testnet getbestblockhash
lumenite-cli -testnet getnetworkinfo
lumenite-cli -testnet getmininginfo

A separate data directory can be specified with:

-datadir=/path/to/lumenite-data
Mining

Lumenite Testnet currently supports development miners for:

CPU mining
NVIDIA CUDA GPU mining

These miners communicate with Lumenite Core and construct HomeScrypt-compatible blocks using work obtained from the node.

Mining software is currently intended for testnet and development use.

Additional hardware compatibility and ASIC-resistance testing is ongoing.

Building Lumenite Core

Lumenite Core can be built from source on Linux.

Typical development build:

./autogen.sh
./configure
make -j$(nproc)

After compilation, the primary binaries are located under:

src/

For example:

src/lumenited --version
src/lumenite-cli --version

Cross-compilation and platform-specific dependencies may require additional configuration.

Development Status

Lumenite is under active development.

Testnet exists to identify consensus, networking, mining, wallet, performance, and compatibility issues before mainnet parameters are finalized.

Current areas of testing include:

LWMA-45 difficulty behavior
sudden hashrate increases and decreases
CPU/GPU competition
ASIC resistance
network synchronization
peer-to-peer operation
wallet behavior
Windows/Linux interoperability
long-running node stability

Mainnet parameters should not be considered final until testnet validation is complete.

Disclaimer

Lumenite Testnet is experimental software.

Testnet LMT has no monetary value.

The protocol, consensus rules, mining algorithm, network parameters, wallet behavior, and software may change during development.

Use testnet software at your own risk.

License

Lumenite Core is distributed under the terms of the MIT software license.

See COPYING for more information.
