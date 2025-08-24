#include "vector.hpp"
#include "deque.hpp"

#include <algorithm>
#include <random>

////////////////////////////////////////
// vector benchmarks
////////////////////////////////////////

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

    // helper lambda: fill with 0..N-1
    auto fillSeq = [N](auto& container) {
        for (size_t i = 0; i < N; ++i)
            container[i] = static_cast<int>(i);
    };

    // helper lambda: timed iteration
    auto timedIterate = [](auto& container) {
        volatile int sum = 0; // volatile prevents optimizing away
        auto start = std::chrono::high_resolution_clock::now();
        for (auto it = container.begin(); it != container.end(); ++it)
            sum += *it;
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };

    //
    // std::vector
    //
    std::vector<int> stdVec(N);
    fillSeq(stdVec);
    auto stdDuration = timedIterate(stdVec);
    std::cout << "std::vector iteration time:  " << stdDuration << " ms\n";

    //
    // rack::vector
    //
    rack::vector<int> rackVec(N, 0);
    fillSeq(rackVec);
    auto rackDuration = timedIterate(rackVec);
    std::cout << "rack::vector iteration time: " << rackDuration << " ms\n";
}

// void vector_benchSort() {
//     const size_t N = 2'000'000;

//     // helper lambda: fill with random ints via push_back
//     auto fillRandom = [N](auto& container) {
//         std::mt19937 rng(42); // fixed seed for reproducibility
//         std::uniform_int_distribution<int> dist(0, 1'000'000);
//         for (size_t i = 0; i < N; ++i)
//             container.push_back(dist(rng));
//     };

//     // helper lambda: timed sort
//     auto timedSort = [](auto& container) {
//         auto start = std::chrono::high_resolution_clock::now();
//         std::sort(container.begin(), container.end());
//         auto end = std::chrono::high_resolution_clock::now();
//         return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//     };

//     //
//     // std::vector
//     //
//     std::vector<int> stdVec;
//     stdVec.reserve(N);
//     fillRandom(stdVec);
//     auto stdDuration = timedSort(stdVec);
//     std::cout << "std::vector sort time:  " << stdDuration << " ms\n";

//     //
//     // rack::vector
//     //
//     rack::vector<int> rackVec;
//     rackVec.reserve(N);
//     fillRandom(rackVec);
//     auto rackDuration = timedSort(rackVec);
//     std::cout << "rack::vector sort time: " << rackDuration << " ms\n";
// }

////////////////////////////////////////
// deque benchmarks
////////////////////////////////////////

//
// Benchmark push_front / push_back
//
void deque_benchPush() {
    const int N = 1'000'000;

    //
    // std::deque
    //
    {
        std::deque<int> stdDeq;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            if (i % 2 == 0)
                stdDeq.push_back(i);
            else
                stdDeq.push_front(i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "std::deque alternating push_front/push_back " << N
                  << " ints: " << elapsed.count() << " seconds\n";
    }

    //
    // rack::deque
    //
    {
        rack::deque<int> rackDeq;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            if (i % 2 == 0)
                rackDeq.push_back(i);
            else
                rackDeq.push_front(i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "rack::deque alternating push_front/push_back " << N
                  << " ints: " << elapsed.count() << " seconds\n";
    }
}

//
// Benchmark iteration
//
void deque_benchIterate() {
    const size_t N = 10'000'000;

    // helper lambda: iterate through a deque-like container and return duration
    auto timedIterate = [](auto& container) {
        volatile int sum = 0; // volatile prevents optimizing away
        auto start = std::chrono::high_resolution_clock::now();
        for (auto it = container.begin(); it != container.end(); ++it)
            sum += *it;
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };

    //
    // std::deque
    //
    std::deque<int> stdDeq;
    for (size_t i = 0; i < N; ++i)
        stdDeq.push_back(static_cast<int>(i));

    auto stdDuration = timedIterate(stdDeq);
    std::cout << "std::deque iteration time:  " << stdDuration << " ms\n";

    //
    // rack::deque
    //
    rack::deque<int> rackDeq;
    for (size_t i = 0; i < N; ++i)
        rackDeq.push_back(static_cast<int>(i));

    auto rackDuration = timedIterate(rackDeq);
    std::cout << "rack::deque iteration time: " << rackDuration << " ms\n";
}

//
// Benchmark sorting
//
void deque_benchSort() {
    const size_t N = 2'000'000;

    // prepare RNG
    std::mt19937 rng(42); // fixed seed for reproducibility
    std::uniform_int_distribution<int> dist(0, 1'000'000);

    //
    // std::deque
    //
    {
        std::deque<int> stdDeq;
        stdDeq.resize(N);
        for (size_t i = 0; i < N; ++i)
            stdDeq[i] = dist(rng);

        auto start = std::chrono::high_resolution_clock::now();
        std::sort(stdDeq.begin(), stdDeq.end());
        auto end = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "[std::deque] sort " << N << " ints: "
                  << elapsed << " ms\n";
    }

    //
    // rack::deque
    //
    {
        rack::deque<int> rackDeq;
        rackDeq.resize(N);
        for (size_t i = 0; i < N; ++i)
            rackDeq[i] = dist(rng);

        auto start = std::chrono::high_resolution_clock::now();
        std::sort(rackDeq.begin(), rackDeq.end());
        auto end = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "[rack::deque] sort " << N << " ints: "
                  << elapsed << " ms\n";
    }
}

//
// Benchmark random access (operator[])
//
void deque_benchRandomAccess() {
    const size_t N = 5'000'000;

    // helper lambda: fill container with 0..N-1 using push_back
    auto fillSeqPushBack = [N](auto& container) {
        for (size_t i = 0; i < N; ++i)
            container.push_back(static_cast<int>(i));
    };

    // helper lambda: timed random access with stride
    auto timedRandomAccess = [N](auto& container) {
        volatile int sum = 0;
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < N; i += 7) // stride of 7 = semi-random access
            sum += container[i];
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    };

    //
    // std::deque
    //
    std::deque<int> stdDeq;
    fillSeqPushBack(stdDeq);
    auto stdDuration = timedRandomAccess(stdDeq);
    std::cout << "std::deque random access time:  " << stdDuration << " us\n";

    //
    // rack::deque
    //
    rack::deque<int> rackDeq;
    fillSeqPushBack(rackDeq);
    auto rackDuration = timedRandomAccess(rackDeq);
    std::cout << "rack::deque random access time: " << rackDuration << " us\n";
}

int main() {
    // vector_benchPushBack();
    // vector_benchmarkIterate();
    // vector_benchSort();

    deque_benchPush();
    deque_benchIterate();
    // deque_benchSort();
    deque_benchRandomAccess();
    return 0;
}