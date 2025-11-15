#include <algorithm>
#include <random>

#include <vector>
#include <deque>
#include <map>

#include "vector.hpp"
#include "shared_ptr.hpp"
#include "unique_ptr.hpp"
#include "deque.hpp"
#include "map.hpp"
#include "priority_queue.hpp"

struct MyClass {
    int val;
    MyClass(int v) : val(v) {}
};

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
        std::cout << "std::vector push_back " << N << " ints: "
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
        std::cout << "rack::vector push_back " << N << " ints: "
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
// shared_ptr benchmarks
////////////////////////////////////////

void shared_ptr_bench() {
    const int N = 1'000'000;

    //
    // std::shared_ptr
    //
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            auto sp = std::make_shared<MyClass>(i);
            if (i % 2 == 0) {
                auto sp2 = sp;              // copy (inc refcount)
            } else {
                sp.reset(new MyClass(i));   // reset with new
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "std::shared_ptr create/copy/reset " << N
                  << " objects: " << elapsed.count() << " seconds\n";
    }

    //
    // rack::shared_ptr
    //
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            auto sp = rack::make_shared<MyClass>(i);
            if (i % 2 == 0) {
                auto sp2 = sp;              // copy (inc refcount)
            } else {
                sp.reset(new MyClass(i));   // reset with new
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "rack::shared_ptr create/copy/reset " << N
                  << " objects: " << elapsed.count() << " seconds\n";
    }
}

////////////////////////////////////////
// unique_ptr benchmarks
////////////////////////////////////////

void unique_ptr_bench() {
    const int N = 1'000'000;

    //
    // std::unique_ptr
    //
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            auto up = std::make_unique<MyClass>(i);
            if (i % 2 == 0) {
                auto up2 = std::move(up);   // move
            } else {
                up.reset(new MyClass(i));  // reset with new
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "std::unique_ptr create/move/reset " << N
                  << " objects: " << elapsed.count() << " seconds\n";
    }

    //
    // rack::unique_ptr
    //
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            auto up = rack::make_unique<MyClass>(i);
            if (i % 2 == 0) {
                auto up2 = std::move(up);   // move
            } else {
                up.reset(new MyClass(i));  // reset with new
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "rack::unique_ptr create/move/reset " << N
                  << " objects: " << elapsed.count() << " seconds\n";
    }
}

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
        std::cout << "std::deque sort " << N << " ints: "
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
        std::cout << "rack::deque sort " << N << " ints: "
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

////////////////////////////////////////
// map benchmarks
////////////////////////////////////////

void map_benchInsert() {
    const int N = 100'000;

    //
    // Benchmark std::map
    //
    {
        std::map<int, int> stdMap;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            stdMap.insert({i, i * 2});
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "std::map insert " << N << " elements: "
                  << elapsed.count() << " seconds\n";
    }

    //
    // Benchmark rack::map
    //
    {
        rack::map<int, int> rackMap;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            rackMap.insert({i, i * 2});
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "rack::map insert " << N << " elements: "
                  << elapsed.count() << " seconds\n";
    }
}

void map_benchIterate() {
    const int N = 1'000'000;

    // helper lambda: fill a map
    auto fillMap = [N](auto& m) {
        for (int i = 0; i < N; ++i)
            m.insert({i, i});
    };

    // helper lambda: timed iteration
    auto timedIterate = [](auto& m) {
        volatile long long sum = 0; // volatile prevents optimizing away
        auto start = std::chrono::high_resolution_clock::now();
        for (auto it = m.begin(); it != m.end(); ++it)
            sum += it->second;
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };

    //
    // std::map
    //
    std::map<int, int> stdMap;
    fillMap(stdMap);
    auto stdDuration = timedIterate(stdMap);
    std::cout << "std::map iteration time:  " << stdDuration << " ms\n";

    //
    // rack::map
    //
    rack::map<int, int> rackMap;
    fillMap(rackMap);
    auto rackDuration = timedIterate(rackMap);
    std::cout << "rack::map iteration time: " << rackDuration << " ms\n";
}

void map_benchLookup() {
    const int N = 500'000;

    std::vector<int> keys(N);
    for (int i = 0; i < N; ++i) keys[i] = i;

    auto timedLookup = [&](auto& m) {
        volatile int found = 0;
        auto start = std::chrono::high_resolution_clock::now();
        for (int k : keys) {
            auto it = m.find(k);
            if (it != m.end()) found += it->second;
        }
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };

    // std::map
    {
        std::map<int,int> stdMap;
        for (int i = 0; i < N; ++i) stdMap.insert({i,i});
        auto t = timedLookup(stdMap);
        std::cout << "std::map lookup " << N << " keys:  " << t << " ms\n";
    }

    // rack::map
    {
        rack::map<int,int> rackMap;
        for (int i = 0; i < N; ++i) rackMap.insert({i,i});
        auto t = timedLookup(rackMap);
        std::cout << "rack::map lookup " << N << " keys: " << t << " ms\n";
    }
}

void map_benchMixedWorkload() {
    const int N = 100'000;

    auto mixedOps = [N](auto& m) {
        // insert half
        for (int i = 0; i < N/2; ++i) m.insert({i,i});

        // lookups + updates
        for (int i = 0; i < N; ++i) {
            auto it = m.find(i % (N/2));
            if (it != m.end()) it->second++;
        }

        // insert rest
        for (int i = N/2; i < N; ++i) m.insert({i,i});
    };

    auto timeIt = [&](auto& m, const char* name) {
        auto start = std::chrono::high_resolution_clock::now();
        mixedOps(m);
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << name << " mixed workload: " << ms << " ms\n";
    };

    std::map<int,int> stdMap;
    rack::map<int,int> rackMap;
    timeIt(stdMap, "std::map");
    timeIt(rackMap, "rack::map");
}

////////////////////////////////////////
// priority_queue benchmarks
////////////////////////////////////////

void priority_queue_benchPushPopContiguous() {
    const int N = 1'000'000;

    //
    // std::priority_queue
    //
    {
        std::priority_queue<int> stdPQ;

        auto start = std::chrono::high_resolution_clock::now();

        // all pushes
        for (int i = 0; i < N; ++i)
            stdPQ.push(i);

        // all pops
        for (int i = 0; i < N; ++i)
            stdPQ.pop();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "std::priority_queue contiguous push+pop " << N
                  << " ints: " << elapsed.count() << " seconds\n";
    }

    //
    // rack::priority_queue
    //
    {
        rack::priority_queue<int> rackPQ;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i)
            rackPQ.push(i);

        for (int i = 0; i < N; ++i)
            rackPQ.pop();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "rack::priority_queue contiguous push+pop " << N
                  << " ints: " << elapsed.count() << " seconds\n";
    }
}

void priority_queue_benchPushPopInterleaved() {
    const int N = 1'000'000;

    //
    // std::priority_queue
    //
    {
        std::priority_queue<int> stdPQ;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            stdPQ.push(i);
            stdPQ.pop();   // pop immediately after each push
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "std::priority_queue interleaved push+pop " << N
                  << " ints: " << elapsed.count() << " seconds\n";
    }

    //
    // rack::priority_queue
    //
    {
        rack::priority_queue<int> rackPQ;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; ++i) {
            rackPQ.push(i);
            rackPQ.pop();
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "rack::priority_queue interleaved push+pop " << N
                  << " ints: " << elapsed.count() << " seconds\n";
    }
}

////////////////////////////////////////
// run
////////////////////////////////////////

void divider(std::string benchName) {
    std::ostringstream oss;
    oss << "\n"
        << "#########################" << "\n"  
        << "## " << benchName << "\n"
        << "#########################" << "\n";
    std::cout << oss.str();
}

void runBenchmarks() {
    // divider("vector");
    // vector_benchPushBack();
    // vector_benchmarkIterate();
    // // vector_benchSort();
    
    // divider("shared_ptr");
    // shared_ptr_bench();

    // divider("unique_ptr");
    // unique_ptr_bench();

    // divider("deque");
    // deque_benchPush();
    // deque_benchIterate();
    // // deque_benchSort();
    // deque_benchRandomAccess();

    // divider("map");
    // map_benchInsert();
    // map_benchIterate();
    // map_benchLookup();
    // map_benchMixedWorkload();

    // divider("priority_queue");
    // priority_queue_benchPushPopContiguous();
    // priority_queue_benchPushPopInterleaved();
}

int main() {
    runBenchmarks();
    return 0;
}