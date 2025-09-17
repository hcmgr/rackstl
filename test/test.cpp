#include <iostream>
#include <cassert>
#include <cmath>
#include <random> 
#include <algorithm>
#include <memory>
#include <gtest/gtest.h>
#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>

#include "vector.hpp"
#include "shared_ptr.hpp"
#include "unique_ptr.hpp"
#include "weak_ptr.hpp"
#include "deque.hpp"
#include "unordered_map.hpp"

//
// Test class. Used to test 'custom object' behaviour of our data structures. 
// Tracks copy / move operations. Implements key operators, hash function, etc.
//
class MyClass {
public:
    int val;
    static int copyCtorCalls;
    static int moveCtorCalls;
    static int copyAssCalls;
    static int moveAssCalls;

    MyClass() : val(0) {}                           
    MyClass(int v) : val(v) {}                      
    MyClass(const MyClass& other) : val(other.val) { ++copyCtorCalls; }
    MyClass(MyClass&& other) noexcept : val(other.val) { ++moveCtorCalls; }

    // Copy assignment
    MyClass& operator=(const MyClass& other) {
        copyAssCalls++;
        if (this != &other) {
            val = other.val;
        }
        return *this;
    }

    // Move assignment
    MyClass& operator=(MyClass&& other) noexcept {
        moveAssCalls++;
        if (this != &other) {
            val = other.val;
        }
        return *this;
    }

    // Comparison
    bool operator==(const MyClass& other) const { return val == other.val; }

    // Increment / Decrement
    MyClass& operator++() { ++val; return *this; }                          // prefix ++
    MyClass operator++(int) { MyClass tmp(*this); ++(*this); return tmp; }  // postfix ++
    MyClass& operator--() { --val; return *this; }                          // prefix --
    MyClass operator--(int) { MyClass tmp(*this); --(*this); return tmp; }  // postfix --

    // assignment from int (useful for random fill)
    MyClass& operator=(int v) { val = v; return *this; }

    // implicit conversion for convenience (debug/printing or mixing with ints)
    operator int() const { return val; }
};

template <>
struct std::hash<MyClass> {
    uint32_t operator()(const MyClass& c) const {
        return std::hash<uint32_t>{}(c.val);
    }
};

int MyClass::copyCtorCalls = 0;
int MyClass::moveCtorCalls = 0;
int MyClass::copyAssCalls = 0;
int MyClass::moveAssCalls = 0;

namespace rack {

////////////////////////////////////////
// vector tests
////////////////////////////////////////

TEST(vector_test, general) {

    //
    // test vector of standard type (i.e. 'int')
    //

    rack::vector<int> vec1;

    EXPECT_TRUE(vec1.empty());
    EXPECT_EQ(vec1.capacity(), 0);
    EXPECT_EQ(vec1.size(), 0);
    EXPECT_EQ(vec1.data(), nullptr);

    int n = 10000;
    for (int i = 0; i < n; i++) {
        vec1.push_back(i);
    }

    int expectedCapacity = 1 << ((int)std::log2(n) + 1);
    EXPECT_EQ(vec1.capacity(), expectedCapacity);
    EXPECT_EQ(vec1.size(), n);
    for (int i = 0; i < n; i++) {
        EXPECT_EQ(vec1[i], i);
    }

    //
    // test vector of non-standard type (i.e. MyClass)
    //

    rack::vector<MyClass> vec2;
    for (int i = 0; i < n; i++) {
        vec2.emplace_back(i);
    }
    expectedCapacity = 1 << ((int)std::log2(n) + 1);
    EXPECT_EQ(vec2.capacity(), expectedCapacity);
    EXPECT_EQ(vec2.size(), n);
    for (int i = 0; i < n; i++) {
        EXPECT_EQ(vec2[i].val, i);
    }
}

TEST(vector_test, iterate) {
    int n = 10;
    rack::vector<int> vec1;
    for (int i = 0; i < n; ++i) {
        vec1.push_back(i);
    }

    // range-based loop
    for (auto &el : vec1) {
        el++;
    }

    // for each
    std::for_each(vec1.begin(), vec1.end(), [](int& w) { w++; });

    // arithmetic and comparison
    EXPECT_NE(vec1.begin(), vec1.end());
    EXPECT_LT(vec1.begin(), vec1.end());
    EXPECT_EQ(vec1.begin() + n, vec1.end());
    EXPECT_EQ(vec1.end() - n, vec1.begin());

    rack::vector<int>::iterator it1 = vec1.begin();
    for (int i = 0; i < n; i++) {
        it1++;
    }
    EXPECT_EQ(it1, vec1.end());

    it1 = vec1.begin();
    it1 += n;
    EXPECT_EQ(it1, vec1.end());

    EXPECT_EQ(n, std::distance(vec1.begin(), vec1.end()));

    // access / dereference
    rack::vector<MyClass> vec2;
    vec2.push_back(MyClass(0));
    rack::vector<MyClass>::iterator it2 = vec2.begin();
    EXPECT_EQ((*it2).val, vec2[0].val);
    EXPECT_EQ(it2->val, vec2[0].val);
    EXPECT_EQ(it2[0].val, vec2[0].val);

    // sort
    rack::vector<int> vec3;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 100);
    for (int i = 0; i < n; ++i) {
        vec3.push_back(dist(gen));
    }
    std::sort(vec3.begin(), vec3.end());
    EXPECT_TRUE(std::is_sorted(vec3.begin(), vec3.end()));
}

////////////////////////////////////////
// shared_ptr tests
////////////////////////////////////////

TEST(shared_ptr_test, general) {
    int val = 1;
    rack::shared_ptr<MyClass> sp = rack::make_shared<MyClass>(val);
    rack::shared_ptr<MyClass> sp1 = sp;
    rack::shared_ptr<MyClass> sp2 = sp;

    EXPECT_EQ(sp.use_count(), 3);
    EXPECT_EQ(sp1.use_count(), 3);
    EXPECT_EQ(sp2.use_count(), 3);

    sp2.reset();
    EXPECT_EQ(sp2.get(), nullptr);
    EXPECT_EQ(sp.use_count(), 2);

    sp1.reset();
    EXPECT_EQ(sp.use_count(), 1);
    EXPECT_TRUE(sp.unique());

    EXPECT_EQ(sp->val, val);
    EXPECT_EQ((*sp).val, val);
    sp.reset();
    EXPECT_FALSE(sp);
    EXPECT_EQ(sp.use_count(), 0);
    sp.reset();

    sp = rack::make_shared<MyClass>(val + 1);
    EXPECT_EQ(sp.use_count(), 1);
    EXPECT_EQ(sp->val, val + 1);

    {
        auto sp3 = sp;
        EXPECT_EQ(sp.use_count(), 2);
    }
    EXPECT_EQ(sp.use_count(), 1);

    sp.reset(new MyClass(val + 2));
    EXPECT_EQ(sp.use_count(), 1);
    EXPECT_EQ(sp->val, val + 2);
}

std::atomic<int> dtorCount{0};
struct Tracker {
    int val;
    Tracker(int v) : val(v) {}
    ~Tracker() { dtorCount.fetch_add(1); }
};

TEST(shared_ptr_test, multithreaded) {
    const int numThreads = 16;
    const int iters = 1000;

    rack::shared_ptr<Tracker> sp = rack::make_shared<Tracker>(42);

    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([sp]() mutable {
            for (int j = 0; j < iters; ++j) {
                auto tmp = sp;
            }
        });
    }

    for (auto &t : threads) t.join();

    // After all threads exit, sp is the only survivor
    EXPECT_TRUE(sp);
    EXPECT_EQ(sp.use_count(), 1);

    // Reset last owner, object should be destroyed exactly once
    sp.reset();
    EXPECT_FALSE(sp);
    EXPECT_EQ(sp.use_count(), 0);
    EXPECT_EQ(dtorCount.load(), 1);
}

////////////////////////////////////////
// unique_ptr tests
////////////////////////////////////////

TEST(unique_ptr_test, general) {
    int val = 1;

    // construct
    rack::unique_ptr<MyClass> p1(new MyClass(val));
    EXPECT_TRUE(p1);
    EXPECT_EQ(p1->val, val);
    EXPECT_EQ((*p1).val, val);

    // move construct
    rack::unique_ptr<MyClass> p1new = std::move(p1);
    EXPECT_FALSE(p1);                 // old one released
    EXPECT_TRUE(p1new);               // new one owns
    EXPECT_EQ(p1new->val, val);

    // move assign
    rack::unique_ptr<MyClass> p2;
    p2 = std::move(p1new);
    EXPECT_FALSE(p1new);
    EXPECT_TRUE(p2);
    EXPECT_EQ(p2->val, val);

    // release
    MyClass* raw = p2.release();
    EXPECT_FALSE(p2);              // p2 no longer owns anything
    EXPECT_EQ(raw->val, val);

    // reset with new object
    p2.reset(new MyClass(val + 1));
    EXPECT_TRUE(p2);
    EXPECT_EQ(p2->val, val + 1);

    // reset to null
    p2.reset();
    EXPECT_FALSE(p2);

    // swap
    rack::unique_ptr<MyClass> p3(new MyClass(val + 2));
    rack::unique_ptr<MyClass> p4(new MyClass(val + 3));
    p3.swap(p4);
    EXPECT_EQ(p3->val, val + 3);
    EXPECT_EQ(p4->val, val + 2);

    // make_unique
    auto p5 = rack::make_unique<MyClass>(val + 4);
    EXPECT_TRUE(p5);
    EXPECT_EQ(p5->val, val + 4);

    // boolean conversion
    EXPECT_TRUE(p5);
    p5.reset();
    EXPECT_FALSE(p5);
}

////////////////////////////////////////
// weak_ptr tests
////////////////////////////////////////

TEST(weak_ptr_test, general) {
    auto sp1 = rack::make_shared<MyClass>(1);
    rack::weak_ptr<MyClass> wp1 = sp1;

    // basic
    ASSERT_EQ(1, wp1.use_count());
    ASSERT_FALSE(wp1.expired());
    wp1.reset();
    ASSERT_FALSE(wp1.expired());
    sp1.reset();
    ASSERT_TRUE(wp1.expired());
    sp1 = rack::make_shared<MyClass>(1);
    wp1 = sp1;
    ASSERT_EQ(1, wp1.use_count());

    // lock
    {
        auto sp2 = wp1.lock();
        ASSERT_EQ(2, wp1.use_count());
    }
    ASSERT_EQ(1, wp1.use_count());

    // swap 
    auto sp3 = rack::make_shared<MyClass>(3);
    rack::weak_ptr<MyClass> wp3 = sp3;
    wp1.swap(wp3);
    ASSERT_EQ(1, wp1.use_count());
    ASSERT_EQ(1, wp3.use_count());
    auto sp1_locked = wp1.lock();
    auto sp3_locked = wp3.lock();
    ASSERT_EQ(3, sp1_locked->val);
    ASSERT_EQ(1, sp3_locked->val);
}

////////////////////////////////////////
// deque tests
////////////////////////////////////////

TEST(deque_test, pushFrontPushBack) {
    int chunkSize = 4 * sizeof(int);
    rack::deque<int> d1(chunkSize); // small chunkSize (for testing purposes)
    std::vector<int> expected;

    // push back and push front
    d1.push_back(2);
    d1.push_back(3);
    d1.push_front(1);
    d1.push_front(0);
    // expect - [0,1,2,3]
    expected = {0,1,2,3};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }
    EXPECT_EQ(d1.front(), 0);
    EXPECT_EQ(d1.back(), 3);

    // pop front and back
    d1.pop_front();
    d1.pop_back();
    // expect - [X,1,2,X]
    expected = {1,2};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }
    EXPECT_EQ(d1.front(), 1);
    EXPECT_EQ(d1.back(), 2);

    // pop remaining
    d1.pop_front();
    d1.pop_front();
    EXPECT_TRUE(d1.empty());
    // EXPECT_TRUE(d1.frontOff == 2 && d1.backOff == 2);

    // push 6 elements
    for (int i = 0; i < 6; i++) { d1.push_back(i); }
    expected = {0,1,2,3,4,5};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }
    EXPECT_EQ(d1.back(), 5);
    // EXPECT_EQ(d1.nChunks, 2);

    // push one more element (forces map growth)
    d1.push_back(6);
    expected = {0,1,2,3,4,5,6};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }
    EXPECT_EQ(d1.back(), 6);
    EXPECT_EQ(d1[d1.size() - 1], 6);
    // EXPECT_EQ(d1.nChunks, 4);
    // EXPECT_EQ(d1.chunkMap[0], nullptr);
}

TEST(deque_test, insertEraseClearShrink) {
    int chunkSize = 5 * sizeof(int);
    rack::deque<int> d1(chunkSize); 
    std::vector<int> expected;

    //
    // insert
    //
    for (int i = 0; i < 8; i++) { d1.push_back(i); }
    expected = {0,1,2,3,4,5,6,7};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }

    d1.insert(d1.begin(), -1);
    expected = {-1,0,1,2,3,4,5,6,7};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }

    d1.insert(d1.begin(), -2);
    d1.insert(d1.begin(), -3);
    expected = {-3,-2,-1,0,1,2,3,4,5,6,7};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }

    d1.insert(d1.begin() + 2, 100);
    expected = {-3,-2,100,-1,0,1,2,3,4,5,6,7};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }

    d1.insert(d1.end() - 2, 100);
    expected = {-3,-2,100,-1,0,1,2,3,4,5,100,6,7};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }

    //
    // erase
    //
    d1.erase(d1.begin());
    expected = {-2,100,-1,0,1,2,3,4,5,100,6,7};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }

    d1.erase(d1.end() - 1);
    expected = {-2,100,-1,0,1,2,3,4,5,100,6};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }

    d1.erase(d1.begin() + 2);
    expected = {-2,100,0,1,2,3,4,5,100,6};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }

    d1.erase(d1.end() - 3);
    expected = {-2,100,0,1,2,3,4,100,6};
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }

    //
    // shrink to fit
    //
    uint32_t oldSize = d1.size();
    d1.shrink_to_fit();
    EXPECT_EQ(d1.size(), oldSize);
    for (size_t i = 0; i < expected.size(); i++) { EXPECT_EQ(d1[i], expected[i]); }
    // EXPECT_EQ(d1.nChunks, 2);

    //
    // clear
    //
    // uint32_t oldNChunks = d1.nChunks;
    d1.clear();
    EXPECT_EQ(d1.size(), 0);
    // EXPECT_EQ(d1.nChunks, oldNChunks);
    EXPECT_EQ(d1.begin(), d1.end());
}

template <typename T>
void deque_testIterateHelper(int chunkSize, int n) {
    rack::deque<T> d(chunkSize);
    for (int i = 0; i < n; i++) { d.push_back(T(i)); }

    EXPECT_NE(d.begin(), d.end());
    for (int i = 0; i < n; i++) {
        EXPECT_EQ(d.begin() + (n - i), d.end() - i);
    }

    auto it = d.begin();
    it += n;
    EXPECT_EQ(it, d.end());
    it -= n;
    EXPECT_EQ(it, d.begin());

    EXPECT_EQ(std::distance(d.begin(), d.end()), n);

    EXPECT_LT(d.begin(), d.end());
    EXPECT_LE(d.begin(), d.end());
    EXPECT_GT(d.end(), d.begin());
    EXPECT_GE(d.end(), d.begin());

    for (int i = 0; i < n; i++) {
        EXPECT_EQ(*(d.begin() + i), T(i));
    }
    for (int i = 1; i <= n; i++) {
        EXPECT_EQ(*(d.end() - i), T(n - i));
    }

    it = d.begin();
    EXPECT_EQ(*(it + 1), T(1));
    for (int i = 0; i < n; i++) {
        EXPECT_EQ(*(it + i), T(i));
        EXPECT_EQ(it[i], T(i));
    }

    for (auto &el : d) { el++; }
    for (int i = 0; i < n; i++) { EXPECT_EQ(d[i]--, T(i + 1)); }

    std::for_each(d.begin(), d.end(), [](T& el) { el++; });
    for (int i = 0; i < n; i++) { EXPECT_EQ(d[i]--, T(i + 1)); }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 100);
    for (int i = 0; i < n; ++i) {
        d[i] = T(dist(gen));
    }
    std::sort(d.begin(), d.end());
    EXPECT_TRUE(std::is_sorted(d.begin(), d.end()));
}

TEST(deque_test, iterate) {
    int chunkSize, n;
    for (int i = 4; i < 16; i++) {
        chunkSize = i * sizeof(int);
        n = chunkSize * 4;

        deque_testIterateHelper<int>(chunkSize, n);
        deque_testIterateHelper<MyClass>(chunkSize, n);
    }
}
};

////////////////////////////////////////
// unordered_map tests
////////////////////////////////////////

TEST(unordered_map_test, general) {
    int N = 2 << 12;
    rack::unordered_map<std::string, int> m(N);
    int oneBeforeMaxLoadFactor = m.maxLoadFactor() * float(N);
    for (int i = 0; i < oneBeforeMaxLoadFactor; i++) {
        m.insert({"monkey" + std::to_string(i), i});
    }

    ASSERT_TRUE(m.size() == oneBeforeMaxLoadFactor);
    ASSERT_TRUE(m.capacity() == N);

    // search
    auto it = m.find("monkey0");
    ASSERT_TRUE(it->first == "monkey0" && it->second == 0);
    ASSERT_TRUE(m.at("monkey0") == 0);

    it = m.find("bonobo0");
    ASSERT_TRUE(it == m.end());
    ASSERT_THROW(m.at("bonobo0"), std::out_of_range);

    ASSERT_TRUE(m.contains("monkey0"));
    ASSERT_TRUE(!m.contains("bonobo0"));

    // insert to cause resize/rehash
    m.insert({"bonobo0", 0});
    ASSERT_TRUE(m.find("bonobo0") != m.end());
    ASSERT_TRUE(m.capacity() == 2*N);

    // clear
    m.clear();
    ASSERT_TRUE(m.size() == 0);
    ASSERT_TRUE(m.empty() == true);

    // [] insert

    // erase

    // verify robin hood internals work correctly (i.e. stealing is correct)
    // note: separate test for this

    // test iterator
    // note: separate test for this

}; // end 'rack'