# HomeScrypt v1.2

Experimental. Sits alongside v1.1, which is unchanged and still what
`primitives/block.cpp` calls. Nothing here affects consensus.

## Why

v1.1's cost is memory latency, as its own source comment says. That is weak
against reconfigurable hardware: an FPGA hides memory latency the same way a
GPU does, by interleaving hash instances, and 16 MiB × several hundred
instances fits easily in the 16 GB of HBM on an Alveo-class card. Capacity
never binds. The arithmetic is close to free in fabric besides — a 64-bit
rotate is a permutation of wires and costs no logic. So the algorithm reduces
to a bandwidth-per-dollar contest, which consumer GPUs win by only ~4x on
capital, most of it given back on power.

FP32 is the lever because it is the one resource a commodity GPU has in
surplus and fabric does not:

| | RTX 4090 (~$1600) | Alveo U55C (~$4000) |
|---|---|---|
| FP32 | ~82 TFLOPS | ~2–3 TFLOPS |
| Bandwidth | 1008 GB/s | 460 GB/s |

The FP gap is an order of magnitude wider than the bandwidth gap. The same
holds against ASICs: Bitmain's Antminer X5/X9 showed that forcing an attacker
to build a CPU is not a defence, since a mining-only RISC-V core beat CPUs at
RandomX by ~100x per box. A GPU is already an ASIC for dense floating point, so
a custom chip has little left to strip out.

This is a cost asymmetry, not an impossibility proof.

## What changes

- `RANDOM_MIX_ROUNDS` drops 16x, `WORDS/2` → `WORDS/32`
- Each remaining round runs a dense binary32 FMA chain
- The chain result folds into both `state[lane]` and `x`

The next round opens with `(state[lane] ^ x ^ round)`, so no memory access in
round N+1 can issue until every FMA in round N has retired. Scratchpad, fill
and fold are unchanged, so memory-hardness is intact.

Rounds had to come down for this to work: v1.1 moves ~256 MB per hash, so an
FPGA would need ~9000 FMAs per round before FP began to bind.

## Determinism

NaN, infinities and denormals are where CPU, CUDA and OpenCL may disagree.
v1.2 makes all three unreachable rather than unlikely.

`safe_f32` forces every operand to a positive normal in `[2^-9, 2^7)`, asserted
over all 2³² inputs in `v12_safe_f32_never_degenerate`. The four exponent bits
are load-bearing: a constant exponent reduces every FMA to fixed point, which
is cheap in fabric.

Division and square root are unused, which also sidesteps OpenCL's 2.5-ulp
default accuracy for both.

Magnitude bound, measured: worst case peaks at 2^70.6 between renormalisations
against binary32's 2^128, and first overflows at `FP_INNER = 16`, so the chosen
8 has 2x margin.

### Build flags

Wrong flags give a different digest, which shows up as rejected shares, not a
compile error.

| Toolchain | Required | Never |
|---|---|---|
| GCC / Clang | `-ffp-contract=off` | `-ffast-math`, `-Ofast` |
| MSVC | `/fp:strict` | `/fp:fast` |
| nvcc | `-fmad=false` | `--use_fast_math` |
| OpenCL | explicit `fma(a,b,c)` | `-cl-fast-relaxed-math` |

`-Ofast` implies `-ffast-math`. GCC defaults to `-ffp-contract=fast` and will
contract `a*b+c` on its own.

## Measured

Reference implementation, scalar, single thread, MSVC `/O2 /fp:strict`.

| Passes | Full hash | FMA/s demanded | 1 year of chain |
|---|---|---|---|
| 0 (no FP) | 13 ms | 0 | 46 min |
| 3 | 15 ms | 0.80 G | 52 min |
| **6 (pinned)** | **20 ms** | **0.90 G** | **1.2 hours** |
| 12 | 33 ms | 0.98 G | 1.9 hours |
| 24 | 57 ms | 1.00 G | 3.3 hours |
| 128 (was) | 238 ms | 1.18 G | 14 hours |

The third column is what matters for the defence; the fourth is what matters
for everyone else.

**The FP32 demand saturates early.** Between 6 and 128 passes the rate an
attacker must sustain rises 31%, while verification cost rises eighteenfold.
An FPGA is stopped by needing enough correctly-rounded FMA units to keep pace
with a GPU. Whether it must do that for 20 ms or 238 ms per hash does not
change how many units it needs.

Verification cost is not paid by miners. It is paid by every node that ever
syncs, for every block ever made. At a 150-second block target a year is about
210,000 blocks: 128 passes is fourteen hours of proof-checking for one year of
history, and it compounds.

128 was chosen optimising only for FPGA resistance, with the node side never
costed. 6 keeps 76% of the pressure for a twelfth of the bill.

### What was tried and did not work

Moving the FP work into the memory latency, so it would overlap the three
dependent scratchpad loads rather than follow them. It makes no difference:
16 MiB fits in a modern L3, so those loads are cache hits and there is no
stall to hide work in. Measured within 2-4% of the plain arrangement at every
pass count, across three runs.

## Evaluating

```
./src/bench/bench_lumenite -filter 'HomeScrypt.*'
```

Pick by the FMA/s column, not by wall time: that is the number an attacker has
to match, and it stops improving long before the clock does.

## Known gaps

- **Verification cost is still the thing to watch.** At 6 passes a header costs
  20 ms, against v1.1's 55 ms -- cheaper than v1.1, not dearer. But validation
  cannot batch across nonces the way mining can, so a pool pays it per share.
  The four independent chains allow ~4-wide SIMD within one hash, which is
  unmeasured. Time IBD before anything else.
- **No pinned test vector.** `v12_deterministic` prints its digest instead of
  asserting one. Reproduce on CUDA and OpenCL first, then pin it.
- **The mix-round count is still a model number.** Only the FP pass count has
  been measured, and the two interact.
- **Versal AI Engines.** Hardened vector FP32 MACs narrow the gap from ~40x to
  ~10x. Blunted by price ($8–15k) and by those units targeting ML dataflow with
  relaxed rounding rather than correctly-rounded IEEE-754 — which is why the
  exact rounding requirement should not be relaxed for speed.
