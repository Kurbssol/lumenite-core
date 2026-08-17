# Build status — HomeLite dev v0.1

- `./autogen.sh`: PASS
- direct compile of `src/crypto/homescrypt.cpp`: PASS using g++ C++17
- deterministic HomeScrypt v0 zero-header vector generated
- full `./configure`: STOPPED because the sandbox lacks the Boost.System link library

The configure failure occurs during dependency discovery (`Could not find a version of the Boost::System library`) before compilation of the node, so it is not currently evidence of a fork source error.

## Next development targets

1. Install/build Litecoin dependencies in a proper Linux build environment and compile the full daemon.
2. Run regtest and mine blocks through the new PoW.
3. Replace inherited Litecoin genesis/checkpoints/assume-valid data.
4. Add a configurable/more memory-hard HomeScrypt v1 implementation.
5. Add a standalone CPU miner and benchmark harness.
6. Add GPU implementation after the consensus test vectors are stable.
