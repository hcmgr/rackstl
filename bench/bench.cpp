#include "vector.hpp"

void vector_benchPushBack() {
    const int N = 1'000'000;

    //
    // Benchmark std::vector
    //
    {
        std::vector<int> stdVec;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            stdVec.push_back(i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "[std::vector] push_back " << N << " ints: "
                  << elapsed.count() << " seconds\n";
    }

    //
    // Benchmark rack::vector
    //
    {
        rack::vector<int> rackVec;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            rackVec.push_back(i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "[rack::vector] push_back " << N << " ints: "
                  << elapsed.count() << " seconds\n";
    }
}

void vector_benchmarkIterate() {
    const size_t N = 10'000'000;

    // fill std::vector
    std::vector<int> stdVec(N);
    for (size_t i = 0; i < N; ++i)
        stdVec[i] = static_cast<int>(i);

    // fill rack::vector
    rack::vector<int> rackVec(N, 0);
    for (size_t i = 0; i < N; ++i)
        rackVec[i] = static_cast<int>(i);

    // std::vector iteration
    auto startStd = std::chrono::high_resolution_clock::now();
    volatile int sumStd = 0;  // volatile to prevent compiler optimizing loop away
    for (auto it = stdVec.begin(); it != stdVec.end(); ++it)
        sumStd += *it;
    auto endStd = std::chrono::high_resolution_clock::now();

    // rack::vector iteration
    auto startRack = std::chrono::high_resolution_clock::now();
    volatile int sumRack = 0;
    for (auto it = rackVec.begin(); it != rackVec.end(); ++it)
        sumRack += *it;
    auto endRack = std::chrono::high_resolution_clock::now();

    // results
    auto stdDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endStd - startStd).count();
    auto rackDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endRack - startRack).count();

    std::cout << "std::vector iteration time:  " << stdDuration << " ms\n";
    std::cout << "rack::vector iteration time: " << rackDuration << " ms\n";
}

int main() {
    vector_benchPushBack();
    vector_benchmarkIterate();
    return 0;
}