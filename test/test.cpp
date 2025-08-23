#include <iostream>
#include <cassert>
#include <cmath>
#include <random> 
#include <algorithm>
#include <memory>
#include <gtest/gtest.h>
#include <deque>
#include <iostream>

#include "vector.hpp"
#include "shared_ptr.hpp"
#include "deque.hpp"

namespace rack {

// Test class used to track copy and move ctor calls when testing our data structures
class MyClass {
public:
    int val;
    static int copyCtorCalls;
    static int moveCtorCalls;

    MyClass(int v) : val(v) {}
    MyClass(const MyClass& other) : val(other.val) { ++copyCtorCalls; }
    MyClass(MyClass&& other) noexcept : val(other.val) { ++moveCtorCalls; }
};
int MyClass::copyCtorCalls = 0;
int MyClass::moveCtorCalls = 0;

////////////////////////////////////////
// vector tests
////////////////////////////////////////

void vector_testGeneral() {

    //
    // test vector of standard type (i.e. 'int')
    //

    rack::vector<int> vec1;

    assert(vec1.empty() == true);
    assert(vec1.capacity() == 0);
    assert(vec1.size() == 0);
    assert(vec1.data() == nullptr);

    int n = 10000;
    for (int i = 0; i < n; i++) {
        vec1.push_back(i);
    }

    int expectedCapacity = 1 << ((int)std::log2(n) + 1);
    assert(vec1.capacity() == expectedCapacity);
    assert(vec1.size() == n);
    for (int i = 0; i < n; i++) {
        assert(vec1[i] == i);
    }

    //
    // test vector of non-standard type (i.e. MyClass)
    //

    rack::vector<MyClass> vec2;

    // use copy and move constructor calls to validate push_back behaviour
    int& copyCount = MyClass::copyCtorCalls = 0;
    int& moveCount = MyClass::moveCtorCalls = 0;
    
    n = 1000;
    for (int i = 0; i < n; i++) {
        int expectedCopyCount = copyCount;
        int expectedMoveCount = moveCount;

        expectedMoveCount += 1; // 1 move into push_back
        expectedCopyCount += 1; // 1 copy into buffer
        if (vec2.capacity() == vec2.size()) { // resize required - n copies into new buffer
            expectedCopyCount += vec2.size();
        }

        MyClass m(i);
        vec2.push_back(std::move(m));

        assert(expectedCopyCount == copyCount);
        assert(expectedMoveCount == moveCount);
    }
}

void vector_testIterate() {
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
    assert(vec1.begin() != vec1.end());
    assert(vec1.begin() < vec1.end());
    assert(vec1.begin() + n == vec1.end());
    assert(vec1.end() - n == vec1.begin());

    rack::vector<int>::iterator it1 = vec1.begin();
    for (int i = 0; i < n; i++) {
        it1++;
    }
    assert(it1 == vec1.end());

    it1 = vec1.begin();
    it1 += n;
    assert(it1 == vec1.end());

    assert(n == std::distance(vec1.begin(), vec1.end()));

    // access / dereference
    rack::vector<MyClass> vec2;
    vec2.push_back(MyClass(0));
    rack::vector<MyClass>::iterator it2 = vec2.begin();
    assert((*it2).val == vec2[0].val);
    assert(it2->val == vec2[0].val);
    assert(it2[0].val == vec2[0].val);

    // sort
    rack::vector<int> vec3;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 100);
    for (int i = 0; i < n; ++i) {
        vec3.push_back(dist(gen));
    }
    std::sort(vec3.begin(), vec3.end());
    assert(std::is_sorted(vec3.begin(), vec3.end()));
}

////////////////////////////////////////
// shared_ptr tests
////////////////////////////////////////

void shared_ptr_testGeneral() {
    int val = 1;
    rack::shared_ptr<MyClass> sp = rack::make_shared<MyClass>(val);
    rack::shared_ptr<MyClass> sp1 = sp;
    rack::shared_ptr<MyClass> sp2 = sp;

    assert(sp.use_count() == 3);
    assert(sp1.use_count() == 3);
    assert(sp2.use_count() == 3);

    sp2.reset();
    assert(sp2.get() == nullptr);
    assert(sp.use_count() == 2);

    sp1.reset();
    assert(sp.use_count() == 1);
    assert(sp.unique());

    assert(sp->val == val);
    assert((*sp).val == val);
    sp.reset();
    assert(!sp);
    assert(sp.use_count() == 0);
    sp.reset();

    sp = rack::make_shared<MyClass>(val + 1);
    assert(sp.use_count() == 1);
    assert(sp->val == val + 1);

    {
        auto sp3 = sp;
        assert(sp.use_count() == 2);
    }
    assert(sp.use_count() == 1);

    sp.reset(new MyClass(val + 2));
    assert(sp.use_count() == 1);
    assert(sp->val == val + 2);
}

////////////////////////////////////////
// deque tests
////////////////////////////////////////

class DequeTests {
public:
    static void deque_testPushFrontAndPushBack() {
        int chunkSize = 4 * sizeof(int);
        rack::deque<int> d1(chunkSize); // small chunkSize (for testing purposes, usually 4KB)

        d1.push_back(2);
        d1.push_back(3);
        d1.push_front(1);
        d1.push_front(0);
        // expect - [0,1,2,3]
        assert(d1.size() == 4);
        assert(d1.front() == 0);
        assert(d1.back() == 3);
        for (int i = 0; i < 4; i++) { assert(d1[i] == i); }

        d1.pop_front();
        d1.pop_back();
        // expect - [X,1,2,X]
        assert(d1.front() == 1);
        assert(d1.back() == 2);

        d1.pop_front();
        d1.pop_front();
        // expect - [X,X,X,X]
        assert(d1.empty());
        assert(d1.frontOff == 2 && d1.backOff == 2);

        for (int i = 0; i < 6; i++) {
            d1.push_back(i);
        }
        // expect - [X,X,0,1] [2,3,4,5]
        assert(d1.size() == 6);
        assert(d1.back() == 5);
        assert(d1.nChunks == 2);

        d1.push_back(6);
        // expect - [] [X,X,0,1] [2,3,4,5] [6,X,X,X]
        assert(d1.size() == 7);
        assert(d1.back() == 6);
        assert(d1[d1.size() - 1] == 6);
        assert(d1.nChunks == 4);
        assert(d1.chunkMap[0] == nullptr);
    }

    template <typename T>
    static void deque_testIterateHelper(rack::deque<T>& d, int chunkSize, int n) {
        std::cout << d.to_string() << "\n";

        //
        // arithmetic and comparison
        //
        assert(d.begin() != d.end());
        for (int i = 0; i < n; i++) {
            assert(d.begin() + (n - i) == d.end() - i);
        }

        auto it = d.begin();
        it += n;
        assert(it == d.end());
        it -= n;
        assert(it == d.begin());

        assert(std::distance(d.begin(), d.end()) == n);

        assert(d.begin() < d.end());
        assert(d.begin() <= d.end());
        assert(d.end() > d.begin());
        assert(d.end() >= d.begin());

        for (int i = 0; i < n; i++) {
            assert(*(d.begin() + i) == i);
        }
        for (int i = 1; i <= n; i++) {
            assert(*(d.end() - i) == n - i);
        }

        // access / dereference
        it = d.begin();
        assert(*(it + 1) == 1);
        for (int i = 0; i < n; i++) {
            assert(*(it + i) == i);
            assert(it[i] == i);
        }

        // looping
        for (auto &el : d) { el++; }
        for (int i = 0; i < n; i++) { assert(d[i]-- == i + 1); }

        std::for_each(d.begin(), d.end(), [](T& el) { el++; });
        for (int i = 0; i < n; i++) { assert(d[i]-- == i + 1); }

        // sort
        rack::deque<int> vec3;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 100);
        for (int i = 0; i < n; ++i) {
            vec3.push_back(dist(gen));
        }
        std::sort(vec3.begin(), vec3.end());
        assert(std::is_sorted(vec3.begin(), vec3.end()));
    }

    static void deque_testIterate() {
        for (int i = 4; i < 16; i++) {
            int chunkSize = i * sizeof(int);
            int n = chunkSize * 4;
            rack::deque<int> d(chunkSize);
            for (int i = 0; i < n; i++) { d.push_back(i); }
            deque_testIterateHelper(d, chunkSize, n);
        }

        // int chunkSize = 4 * sizeof(int);
        // int n = 10;
        // rack::deque<int> d(chunkSize);
        // for (int i = 0; i < n; i++) { d.push_back(i); }
        // deque_testIterateHelper(d, chunkSize, n); 
    }

    static void deque_testInsertErase() {
        int chunkSize = 8 * sizeof(int);
        rack::deque<int> d1(chunkSize); 
        int n = 12;
        for (int i = 0; i < n; i++) { d1.push_back(i); }
        std::cout << d1.to_string() << "\n";

        d1.insert(d1.begin(), -1);
        std::cout << d1.to_string() << "\n";

        d1.insert(d1.begin() + 4, 420);
        std::cout << d1.to_string() << "\n";
        d1.insert(d1.begin() + 4, 421);
        std::cout << d1.to_string() << "\n";
        d1.insert(d1.begin() + 4, 422);
        std::cout << d1.to_string() << "\n";
        d1.insert(d1.begin() + 4, 423);
        std::cout << d1.to_string() << "\n";
    }
};
};

int main() {
    // rack::DequeTests::deque_testPushFrontAndPushBack(); 
    // rack::DequeTests::deque_testIterate();
    rack::DequeTests::deque_testInsertErase();
    return 0;
}