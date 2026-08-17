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
