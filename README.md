# Lumenite Core

**Lumenite (LMT)** is an experimental proof-of-work cryptocurrency focused on bringing cryptocurrency mining back to general-purpose home hardware.

Lumenite is derived from Litecoin Core and replaces Litecoin's block proof-of-work with **HomeScrypt v1.1**, a memory-hard proof-of-work algorithm designed for CPU and GPU mining.

> **PRE-LAUNCH NOTICE**
>
> Lumenite mainnet has **not launched yet**.
>
> The genesis block currently contained in the `mainnet-v1` branch is a **development genesis block only** and will NOT be the genesis block used by the final Lumenite mainnet.
>
> The final mainnet genesis block will be generated and committed as part of the official launch process. Do not treat the current development chain as the official Lumenite blockchain.

---

## Lumenite

| | |
|---|---|
| **Name** | Lumenite |
| **Ticker** | LMT |
| **Consensus** | Proof of Work |
| **PoW Algorithm** | HomeScrypt v1.1 |
| **Target Block Time** | 150 seconds |
| **Initial Block Reward** | 50 LMT |
| **Halving Interval** | 840,000 blocks |
| **Nominal Maximum Supply** | 84,000,000 LMT |
| **Difficulty Adjustment** | LWMA-45 |
| **Mainnet Address HRP** | `lmt` |
| **Premine** | None |

## HomeScrypt

HomeScrypt is Lumenite's custom proof-of-work algorithm.

The current implementation uses a 16 MiB scratchpad per mining job/thread and combines an initial Scrypt stage with additional memory filling, randomized mixing, folding, and final SHA-256 processing.

HomeScrypt v1.1 has been implemented and tested with CPU and GPU miners.

Existing Litecoin Scrypt ASIC work is not compatible with Lumenite HomeScrypt proof-of-work.

HomeScrypt should be considered an experimental proof-of-work design. No claim is made that specialized hardware for HomeScrypt can never be developed.

## Mining

Lumenite is intended to support mining on commonly available hardware, including:

- CPUs
- NVIDIA GPUs
- AMD GPUs

Mining software and official mainnet connection information will be published for the Lumenite mainnet launch.

The public mainnet mining pool will be operated separately by **KCMiners**.

## Network

Lumenite uses a Litecoin Core-derived blockchain architecture with Lumenite-specific consensus and network parameters.

Mainnet is currently **pre-launch**.

Public mainnet peer-to-peer networking and mining services will remain unavailable until the official launch.

The final mainnet genesis block has **not been published or finalized**.

## Difficulty Adjustment

Lumenite uses a per-block **LWMA-45** difficulty adjustment algorithm.

The algorithm uses a rolling window of 45 blocks and is intended to respond to changes in network hashrate more quickly than Litecoin's original difficulty adjustment schedule.

## Development Status

The `mainnet-v1` branch contains the current pre-launch Lumenite Core 1.0 development code.

Current development work includes:

- HomeScrypt v1.1 consensus proof-of-work
- CPU mining support
- NVIDIA GPU mining support
- AMD GPU mining support
- Lumenite network parameters
- LWMA-45 difficulty adjustment
- Lumenite address formats
- Lumenite RPC updates
- Mainnet infrastructure preparation
- Block explorer infrastructure
- SOLO mining pool infrastructure

The current mainnet genesis values are temporary development values.

## Repository Branches

### `mainnet-v1`

Current Lumenite mainnet development branch.

**This branch is pre-launch and currently contains a temporary development genesis block.**

### Testnet

Lumenite has been tested on a separate test network using HomeScrypt mining, difficulty adjustment, CPU/GPU miners, Stratum infrastructure, and block explorer infrastructure.

Testnet and mainnet are separate networks.

## Mainnet Launch

The final Lumenite mainnet launch process will include:

1. Generate the final mainnet genesis block.
2. Hardcode and verify the final genesis parameters.
3. Perform a clean Lumenite Core build.
4. Verify the final blockchain and network parameters.
5. Publish the final source code and release.
6. Publish official wallets and mining software.
7. Enable public mainnet peer-to-peer networking.
8. Enable public mining infrastructure.
9. Begin mining from mainnet block 1.

No mainnet blocks are intended to be mined before the official public launch.

## Software

Lumenite Core provides the primary node, wallet, RPC, and blockchain functionality for the Lumenite network.

Primary binaries include:

- `lumenited`
- `lumenite-cli`
- `lumenite-wallet`
- `lumenite-tx`
- Lumenite Qt wallet

## License

Lumenite Core is released under the terms of the MIT license.

See [COPYING](COPYING) for additional information.

## Upstream

Lumenite Core is derived from Litecoin Core.

Copyright and licensing notices from upstream Bitcoin Core and Litecoin Core are retained where required.

---

**Lumenite mainnet is not live.**

Official mainnet genesis information, releases, downloads, mining software, network endpoints, and project website information will be published at launch.
