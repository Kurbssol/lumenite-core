# HomeLite development fork

This tree is an experimental Litecoin-derived blockchain used to develop and test a CPU/GPU-oriented proof-of-work.

## Fork status

- Working name: **HomeLite** (temporary)
- Upstream: Litecoin Core 0.21.5.6 source snapshot supplied 2026-08-16
- Block format: unchanged Litecoin-style 80-byte header for v0
- PoW: changed from Litecoin Scrypt to experimental **HomeScrypt v0**
- Main P2P magic: `a7 3c 91 e5`
- Main P2P port: `17333`
- Test P2P port: `17335`
- Regtest P2P port: `17444`
- Main Bech32 HRP: `hl`
- Litecoin DNS/fixed seeds: removed
- Genesis: still Litecoin genesis during early development; replace before any public testnet/mainnet

## HomeScrypt v0

`header -> scrypt -> domain-separated SHA256 mix -> scrypt -> SHA256`

This first implementation intentionally reuses Litecoin's audited Scrypt primitive twice so the chain can be brought up quickly. It changes the valid PoW function immediately, but it is **not claimed to be ASIC-resistant yet**. We will replace/tune the internal memory-hard stage as benchmarks are added.

## Safety

Do not connect this experimental tree to real funds. Do not launch a public network with the inherited genesis/checkpoints/chain assumptions until those are replaced and consensus tests pass.
