# Lumenite Core

**Lumenite (LMT)** is a Proof-of-Work cryptocurrency built around the **HomeScrypt v1.1** mining algorithm.

The project is currently in **public testnet development**.

HomeScrypt is designed for CPU and GPU mining, with the goal of making mining accessible to home users while maintaining a Bitcoin-style UTXO blockchain and fixed monetary policy.

---

## Project Status

Current status:

**Lumenite Testnet v0.1**

Mainnet has **not launched**.

The current testnet is being used to validate:

- HomeScrypt v1.1 consensus
- CPU mining
- NVIDIA CUDA mining
- block propagation
- multi-node synchronization
- chain reorganizations
- stale-work handling
- wallet operation
- transaction propagation
- difficulty adjustment behavior
- Windows and Linux compatibility
- mining performance across different hardware

Testnet coins have no monetary value.

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
| Coinbase Maturity | 100 blocks |
| Premine | None |
| Developer Reward | None |
| MWEB | Disabled |

The current Testnet v0.1 difficulty adjustment uses a 2,016-block / 3.5-day retarget window.

The final mainnet difficulty-adjustment algorithm is still being evaluated during testnet.

See:

`LUMENITE_CONSENSUS.md`

for the full consensus specification.

---

## HomeScrypt v1.1

HomeScrypt v1.1 is the Proof-of-Work algorithm used by Lumenite.

Development testing has successfully demonstrated:

- CPU mining
- NVIDIA CUDA mining
- independent miners competing for blocks
- valid block submission
- stale-work detection
- temporary chain forks
- automatic chain reorganization
- multi-node synchronization

The Lumenite Core consensus implementation is authoritative for block validity.

---

## Lumenite Core Binaries

The Lumenite Core source builds the following primary applications:

```text
lumenited
lumenite-cli
lumenite-wallet
lumenite-tx
lumenite-qt
