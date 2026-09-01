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

Scalar, single thread, MSVC `/O2 /fp:strict`, `scrypt_1024_1_1_256` stubbed
(fixed cost in both versions):

| Setting | s/hash | FMA/hash | vs v1.1 |
|---|---|---|---|
| v1.1 | 0.0551 | — | 1.0x |
| `FP0` (memory floor) | 0.0131 | 0 | 0.24x |
| `FP32` | 0.0717 | 6.7e7 | 1.3x |
| `FP64` | 0.1295 | 1.3e8 | 2.4x |
| **`FP128` (pinned)** | **0.2380** | **2.7e8** | **4.3x** |
| `FP256` | 0.4540 | 5.4e8 | 8.2x |
| `FP512` | 0.8904 | 1.07e9 | 16.2x |

Linear above `FP32`, confirming the phase is FP-bound. A large FPGA becomes
FP-bound near 53 passes, so 128 clears it with margin at 4.3x rather than 16x.

## Evaluating

```
./src/bench/bench_lumenite -filter 'HomeScrypt.*'
```

Pick the largest setting that has not yet bent the CPU curve. The CPU knees
before the GPU does, so it is the binding constraint, not the FPGA.

## Known gaps

- **Verification cost.** At `FP128` a header costs 4.3x v1.1 to check, and
  validation cannot batch across nonces the way mining can. The four
  independent chains allow ~4-wide SIMD within one hash, which should recover
  much of it, but that is unmeasured. Time IBD before anything else.
- **No pinned test vector.** `v12_deterministic` prints its digest instead of
  asserting one. Reproduce on CUDA and OpenCL first, then pin it.
- **The mix-round count is still a model number.** Only the FP pass count has
  been measured, and the two interact.
- **Versal AI Engines.** Hardened vector FP32 MACs narrow the gap from ~40x to
  ~10x. Blunted by price ($8–15k) and by those units targeting ML dataflow with
  relaxed rounding rather than correctly-rounded IEEE-754 — which is why the
  exact rounding requirement should not be relaxed for speed.
