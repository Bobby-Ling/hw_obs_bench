#include "huawei_obs.h"
#include <atomic>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <eSDKOBS.h>
#include <gtest/gtest.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class Tracer {
  public:
    Tracer(const std::string &filename = "results.csv") : filename(filename) {}
    struct DataFrameRow {
        std::string prefix;
        std::size_t threads;
        std::size_t object_size;
        std::size_t total_ops;
        std::size_t loop_count;
        double seconds;
        double ops_per_s;
        double mb_per_s;
        double lat_p50;
        double lat_p90;
        double lat_p99;
    };

    DataFrameRow append_row(
        std::string prefix,
        std::size_t threads,
        std::size_t object_size,
        std::size_t loop_count,
        double seconds,
        const std::vector<double> &latencies
    ) {
        std::size_t total_ops = latencies.size();
        DataFrameRow row{
            .prefix = prefix,
            .threads = threads,
            .object_size = object_size,
            .total_ops = total_ops,
            .loop_count = loop_count,
            .seconds = seconds,
            .ops_per_s = total_ops / seconds,
            .mb_per_s = (total_ops * object_size) / (1024.0 * 1024.0) / seconds,
            .lat_p50 = get_percentile(latencies, 0.50),
            .lat_p90 = get_percentile(latencies, 0.90),
            .lat_p99 = get_percentile(latencies, 0.99),
        };
        std::unique_lock<std::mutex> lock(mutex_);
        rows_.push_back(row);
        return row;
    }

    std::string to_csv() {
        std::unique_lock<std::mutex> lock(mutex_);
        std::string buffer;
        buffer += "prefix,threads,object_size,total_ops,loop_count,seconds,ops_per_s,mb_per_s,lat_p50,lat_p90,lat_p99\n";
        for (const auto &row : rows_) {
            buffer += fmt::format(
                "{},{},{},{},{},{:.6f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f}\n",
                row.prefix,
                row.threads,
                row.object_size,
                row.total_ops,
                row.loop_count,
                row.seconds,
                row.ops_per_s,
                row.mb_per_s,
                row.lat_p50,
                row.lat_p90,
                row.lat_p99
            );
        }
        return buffer;
    }

    void save_csv() {
        std::ofstream ofs(filename);
        ofs << to_csv();
        ofs.close();
    }

    double get_percentile(std::vector<double> latencies, double p) {
        if (latencies.empty())
            return 0.0;
        std::sort(latencies.begin(), latencies.end());
        size_t idx = static_cast<size_t>(latencies.size() * p);
        if (idx >= latencies.size())
            idx = latencies.size() - 1;
        return latencies[idx];
    }

  private:
    std::vector<DataFrameRow> rows_;
    std::mutex mutex_;

    std::string filename;
};

class OBSBenchmark : public benchmark::Fixture {
  public:
    void SetUp(const ::benchmark::State &state) override {}

    void TearDown(const ::benchmark::State &state) override {}

  protected:
    const HuaweiCloudObs obs_client;
    Tracer tracer;

    static inline std::size_t LOOP_MIN = 10;

    static inline std::size_t LOOP_MAX = 1000;

    static inline std::size_t LOOP_COUNT = 16;

    std::atomic<uint64_t> key_counter{0};

    std::size_t get_loop_count(std::size_t threads, std::size_t object_size) const {
        // const int64_t N = LOOP_MIN;
        // const int64_t total_size_to_write = 16LL * N * (1LL << 30);
        // const int64_t loop_max = LOOP_MAX;
        // int64_t loop_count = total_size_to_write / (threads * object_size);
        // loop_count = std::min(loop_count, loop_max);
        // if (loop_count == 0) loop_count = 1;
        // return loop_count;
        // 计算分布, 使用固定的操作数
        return LOOP_COUNT;
    }
};

// 生成指定大小的测试数据
std::string generate_data(size_t size) {
    std::string data(size, 'a');
    return data;
}

BENCHMARK_DEFINE_F(OBSBenchmark, put_object)(benchmark::State &state) {
    const auto object_size = state.range(0);
    const auto num_threads = state.range(1);
    std::string data = generate_data(object_size);

    const auto loop_count = get_loop_count(num_threads, object_size);

    std::string prefix = "put_object";
    // 为每个<thread_index, loop_index>创建一个唯一的 key
    std::vector<std::vector<std::string>> keys(num_threads, std::vector<std::string>(loop_count));
    for (int i = 0; i < num_threads; ++i) {
        for (int j = 0; j < loop_count; ++j) {
            keys[i][j] = prefix + "_" + std::to_string(key_counter++);
        }
    }

    // 只执行一次
    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        std::vector<double> group_latencies;
        std::mutex lat_mutex;

        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i, loop_count]() {
                std::vector<double> thread_local_latencies;
                thread_local_latencies.reserve(loop_count);
                for (int j = 0; j < loop_count; ++j) {
                    auto t1 = std::chrono::high_resolution_clock::now();

                    obs_client.put_object(keys[i][j], data);
                    // std::this_thread::sleep_for(std::chrono::milliseconds(1 + j));

                    auto t2 = std::chrono::high_resolution_clock::now();
                    double lat_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
                    thread_local_latencies.push_back(lat_ms);
                }
                std::lock_guard<std::mutex> lock(lat_mutex);
                group_latencies.insert(group_latencies.end(), thread_local_latencies.begin(), thread_local_latencies.end());
            });
        }

        for (auto &t : threads) {
            t.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        double duration_sec = std::chrono::duration<double>(end_time - start_time).count();
        tracer.append_row(
            prefix,
            num_threads,
            object_size,
            loop_count,
            duration_sec,
            group_latencies
        );
        tracer.save_csv();
    }
}

BENCHMARK_DEFINE_F(OBSBenchmark, append_object)(benchmark::State &state) {
    const auto object_size = state.range(0);
    const auto num_threads = state.range(1);
    std::string data = generate_data(object_size);

    const auto loop_count = get_loop_count(num_threads, object_size);

    std::string prefix = "append_object";
    // 为每个thread创建一个唯一的 key
    std::vector<std::string> keys(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        keys[i] = prefix + "_" + std::to_string(key_counter++);
    }

    // 只执行一次
    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        std::vector<double> group_latencies;
        std::mutex lat_mutex;

        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i, loop_count]() {
                std::vector<double> thread_local_latencies;
                thread_local_latencies.reserve(loop_count);
                static std::size_t next_start_pos = 0;
                for (int j = 0; j < loop_count; ++j) {
                    auto t1 = std::chrono::high_resolution_clock::now();

                    next_start_pos = obs_client.append_object(keys[i], data, next_start_pos);
                    // std::this_thread::sleep_for(std::chrono::milliseconds(1 + j));

                    auto t2 = std::chrono::high_resolution_clock::now();
                    double lat_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
                    thread_local_latencies.push_back(lat_ms);
                }
                std::lock_guard<std::mutex> lock(lat_mutex);
                group_latencies.insert(group_latencies.end(), thread_local_latencies.begin(), thread_local_latencies.end());
            });
        }

        for (auto &t : threads) {
            t.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        double duration_sec = std::chrono::duration<double>(end_time - start_time).count();
        tracer.append_row(
            prefix,
            num_threads,
            object_size,
            loop_count,
            duration_sec,
            group_latencies
        );
        tracer.save_csv();
    }
}

// loop_min=N               最少循环次数
// loop_max=1000            最大循环次数
// size=128*128*N=16N GB    最大写入大小
// threads=1-128            并发(2x)
// object_size=1KB-128MB    访问粒度(2x)
// 比如, threads=128, object_size=128MB时:
// put_object(content=data[size=128MB])             N次
// append_object(append_content=data[size=128MB])   N次
// 测试组<threads, object_size>之间尽可能维持写入总量16N GB不变,
// 但是如果参数过小
// <threads=1, object_size=1KB>时候每个线程会执行约1.6亿次
// 因此仿照fio的number_ios / runtime 控制最大循环次数 / 最大运行时间
//
// 或者每个测试组<threads, object_size>执行固定次数?
//
// 比较的是什么:
// put_object(content=data[size])和append_object(append_content=data[size])

// BENCHMARK_REGISTER_F(OBSBenchmark, put_object)
//     ->RangeMultiplier(2)
//     ->Ranges({
//         {1 << 12, 128 << 18},  // 4KB to 32MB
//         {4, 32}               // 4 to 32 threads
//     })
//     // ->Ranges({
//     //     {1 << 10, 128 << 20},  // 1KB to 128MB
//     //     {1, 128}               // 1 to 128 threads
//     // })
//     ->Iterations(1)
//     ->UseRealTime()
//     ->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(OBSBenchmark, append_object)
    ->RangeMultiplier(2)
    ->Ranges({
        {1 << 12, 128 << 18},  // 4KB to 32MB
        {4, 32}               // 4 to 32 threads
    })
    ->Iterations(1)
    ->Threads(1)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

int main(int argc, char **argv) {
    init_logger();
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;
    ::benchmark::RunSpecifiedBenchmarks();
}