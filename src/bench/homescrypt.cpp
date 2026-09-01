// HomeScrypt v1.1 CPU benchmarks.
//
// HomeScryptV11 measures single-thread latency.
// HomeScryptV11_MT{1,2,4,8,16} measures aggregate throughput with
// multiple concurrent hashing threads. Each benchmark iteration performs
// one HomeScrypt hash per worker thread.

#include <bench/bench.h>
#include <crypto/homescrypt.h>

#include <array>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

void SetNonce(std::array<unsigned char, 80>& header, std::uint32_t nonce)
{
    header[76] = static_cast<unsigned char>(nonce);
    header[77] = static_cast<unsigned char>(nonce >> 8);
    header[78] = static_cast<unsigned char>(nonce >> 16);
    header[79] = static_cast<unsigned char>(nonce >> 24);
}

template <std::size_t THREADS>
void HomeScryptV11MultiThread(benchmark::Bench& bench)
{
    // Each worker gets its own header and digest buffer. The HomeScrypt
    // implementation allocates its own 16 MiB scratchpad per call.
    std::array<std::array<unsigned char, 80>, THREADS> headers{};
    std::array<std::array<unsigned char, 32>, THREADS> outputs{};
    std::uint32_t batch_nonce = 0;

    bench.batch(THREADS).unit("hash").run([&] {
        std::vector<std::thread> workers;
        workers.reserve(THREADS);

        for (std::size_t i = 0; i < THREADS; ++i) {
            // Give every worker a distinct nonce/work item.
            SetNonce(headers[i], batch_nonce + static_cast<std::uint32_t>(i));

            workers.emplace_back([&, i] {
                homescrypt_v11(
                    reinterpret_cast<const char*>(headers[i].data()),
                    reinterpret_cast<char*>(outputs[i].data())
                );
            });
        }

        for (auto& worker : workers) {
            worker.join();
        }

        batch_nonce += static_cast<std::uint32_t>(THREADS);
    });
}

} // namespace

static void HomeScryptV11(benchmark::Bench& bench)
{
    std::array<unsigned char, 80> header{};
    std::array<unsigned char, 32> output{};
    std::uint32_t nonce = 0;

    bench.unit("hash").run([&] {
        SetNonce(header, nonce);

        homescrypt_v11(
            reinterpret_cast<const char*>(header.data()),
            reinterpret_cast<char*>(output.data())
        );

        ++nonce;
    });
}

static void HomeScryptV11_MT1(benchmark::Bench& bench)
{
    HomeScryptV11MultiThread<1>(bench);
}

static void HomeScryptV11_MT2(benchmark::Bench& bench)
{
    HomeScryptV11MultiThread<2>(bench);
}

static void HomeScryptV11_MT4(benchmark::Bench& bench)
{
    HomeScryptV11MultiThread<4>(bench);
}

static void HomeScryptV11_MT8(benchmark::Bench& bench)
{
    HomeScryptV11MultiThread<8>(bench);
}

static void HomeScryptV11_MT16(benchmark::Bench& bench)
{
    HomeScryptV11MultiThread<16>(bench);
}

BENCHMARK(HomeScryptV11);
BENCHMARK(HomeScryptV11_MT1);
BENCHMARK(HomeScryptV11_MT2);
BENCHMARK(HomeScryptV11_MT4);
BENCHMARK(HomeScryptV11_MT8);
BENCHMARK(HomeScryptV11_MT16);

// HomeScrypt v1.2 benchmarks. The _MT variants mirror the v1.1 set so both
// algorithms can be compared in one run.
//
// The _FP* sweep holds mix rounds fixed and varies only the FP32 work, showing
// where a machine stops being memory-bound. FP0 is the memory floor. Pick the
// largest setting that has not yet bent the CPU curve: the CPU knees before
// the GPU does, so it is the binding constraint.

namespace {

template <std::size_t THREADS>
void HomeScryptV12MultiThread(benchmark::Bench& bench)
{
    std::array<std::array<unsigned char, 80>, THREADS> headers{};
    std::array<std::array<unsigned char, 32>, THREADS> outputs{};
    std::uint32_t batch_nonce = 0;

    bench.batch(THREADS).unit("hash").run([&] {
        std::vector<std::thread> workers;
        workers.reserve(THREADS);

        for (std::size_t i = 0; i < THREADS; ++i) {
            SetNonce(headers[i], batch_nonce + static_cast<std::uint32_t>(i));

            workers.emplace_back([&, i] {
                homescrypt_v12(
                    reinterpret_cast<const char*>(headers[i].data()),
                    reinterpret_cast<char*>(outputs[i].data())
                );
            });
        }

        for (auto& worker : workers) {
            worker.join();
        }

        batch_nonce += static_cast<std::uint32_t>(THREADS);
    });
}

// Single-threaded, with the FP work varied and the memory work held constant.
void HomeScryptV12Sweep(benchmark::Bench& bench, std::size_t fp_passes)
{
    // Matches RANDOM_MIX_ROUNDS_V12: 16 MiB / 8 bytes / 32.
    constexpr std::size_t MIX_ROUNDS = (16u * 1024u * 1024u / 8u) / 32u;

    std::array<unsigned char, 80> header{};
    std::array<unsigned char, 32> output{};
    std::uint32_t nonce = 0;

    bench.unit("hash").run([&] {
        SetNonce(header, nonce);

        homescrypt_v12_tuned(
            reinterpret_cast<const char*>(header.data()),
            reinterpret_cast<char*>(output.data()),
            MIX_ROUNDS, fp_passes
        );

        ++nonce;
    });
}

} // namespace

static void HomeScryptV12(benchmark::Bench& bench)
{
    std::array<unsigned char, 80> header{};
    std::array<unsigned char, 32> output{};
    std::uint32_t nonce = 0;

    bench.unit("hash").run([&] {
        SetNonce(header, nonce);

        homescrypt_v12(
            reinterpret_cast<const char*>(header.data()),
            reinterpret_cast<char*>(output.data())
        );

        ++nonce;
    });
}

static void HomeScryptV12_MT1(benchmark::Bench& bench) { HomeScryptV12MultiThread<1>(bench); }
static void HomeScryptV12_MT2(benchmark::Bench& bench) { HomeScryptV12MultiThread<2>(bench); }
static void HomeScryptV12_MT4(benchmark::Bench& bench) { HomeScryptV12MultiThread<4>(bench); }
static void HomeScryptV12_MT8(benchmark::Bench& bench) { HomeScryptV12MultiThread<8>(bench); }
static void HomeScryptV12_MT16(benchmark::Bench& bench) { HomeScryptV12MultiThread<16>(bench); }

// FMAs per hash = 65536 mix rounds * fp_passes * 32.
static void HomeScryptV12_FP0(benchmark::Bench& b)    { HomeScryptV12Sweep(b, 0); }     // memory floor
static void HomeScryptV12_FP128(benchmark::Bench& b)  { HomeScryptV12Sweep(b, 128); }   // 2.7e8, pinned default
static void HomeScryptV12_FP256(benchmark::Bench& b)  { HomeScryptV12Sweep(b, 256); }   // 5.4e8
static void HomeScryptV12_FP512(benchmark::Bench& b)  { HomeScryptV12Sweep(b, 512); }   // 1.1e9
static void HomeScryptV12_FP1024(benchmark::Bench& b) { HomeScryptV12Sweep(b, 1024); }  // 2.1e9

BENCHMARK(HomeScryptV12);
BENCHMARK(HomeScryptV12_MT1);
BENCHMARK(HomeScryptV12_MT2);
BENCHMARK(HomeScryptV12_MT4);
BENCHMARK(HomeScryptV12_MT8);
BENCHMARK(HomeScryptV12_MT16);
BENCHMARK(HomeScryptV12_FP0);
BENCHMARK(HomeScryptV12_FP128);
BENCHMARK(HomeScryptV12_FP256);
BENCHMARK(HomeScryptV12_FP512);
BENCHMARK(HomeScryptV12_FP1024);
