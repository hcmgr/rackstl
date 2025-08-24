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

//
// Test class used to track copy and move ctor calls when testing our data structures
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

int MyClass::copyCtorCalls = 0;
int MyClass::moveCtorCalls = 0;
int MyClass::copyAssCalls = 0;
int MyClass::moveAssCalls = 0;

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

    // //
    // // test vector of non-standard type (i.e. MyClass)
    // //

    // rack::vector<MyClass> vec2;

    // // use copy and move constructor calls to validate push_back behaviour
    // int& copyCtorCount = MyClass::copyCtorCalls = 0;
    // int& moveCtorCount = MyClass::moveCtorCalls = 0;
    // int& copyAssCount = MyClass::copyCtorCalls = 0;
    // int& moveAssCount = MyClass::moveCtorCalls = 0;
    
    // n = 100;
    // for (int i = 0; i < n; i++) {
    //     int expCopyCount = copyCtorCount + copyAssCount;
    //     expCopyCount += 2;

    //     if (vec2.capacity() == vec2.size()) { // resize required - n copies into new buffer
    //         expCopyCount += vec2.size();
    //     }

    //     MyClass m(i);
    //     vec2.push_back(std::move(m));

    //     std::cout << expCopyCount << " " << copyCtorCount + copyAssCount << "\n";
    //     assert(expCopyCount == copyCtorCount + copyAssCount);
    // }
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
    void deque_testPushFrontPushBack() {
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
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }
        assert(d1.front() == 0);
        assert(d1.back() == 3);

        // pop front and back
        d1.pop_front();
        d1.pop_back();
        // expect - [X,1,2,X]
        expected = {1,2};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }
        assert(d1.front() == 1);
        assert(d1.back() == 2);

        // pop remaining
        d1.pop_front();
        d1.pop_front();
        // expect - [X,X,X,X]
        assert(d1.empty());
        assert(d1.frontOff == 2 && d1.backOff == 2);

        // push 6 elements
        for (int i = 0; i < 6; i++) { d1.push_back(i); }
        // expect - [X,X,0,1] [2,3,4,5]
        expected = {0,1,2,3,4,5};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }
        assert(d1.back() == 5);
        assert(d1.nChunks == 2);

        // push one more element (forces map growth)
        d1.push_back(6);
        // expect - [] [X,X,0,1] [2,3,4,5] [6,X,X,X]
        expected = {0,1,2,3,4,5,6};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }
        assert(d1.back() == 6);
        assert(d1[d1.size() - 1] == 6);
        assert(d1.nChunks == 4);
        assert(d1.chunkMap[0] == nullptr);
    }

    void deque_testInsertErase() {
        int chunkSize = 5 * sizeof(int);
        rack::deque<int> d1(chunkSize); 
        std::vector<int> expected;

        //
        // insert
        //

        for (int i = 0; i < 8; i++) { d1.push_back(i); }
        // initial - [0,0,0,1,2], [3,4,5,6,7]
        expected = {0,1,2,3,4,5,6,7};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }

        // insert at front
        d1.insert(d1.begin(), -1);
        // expect - [0,-1,0,1,2], [3,4,5,6,7] 
        expected = {-1,0,1,2,3,4,5,6,7};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }

        // insert at front and back such that it grows
        d1.insert(d1.begin(), -2);
        d1.insert(d1.begin(), -3);
        // expect - [0,0,0,0,-3], [-2,-1,0,1,2], [3,4,5,6,7], []
        expected = {-3,-2,-1,0,1,2,3,4,5,6,7};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }

        // insert causing shift left
        d1.insert(d1.begin() + 2, 100);
        // expect - [0,0,0,-3,-2], [100,-1,0,1,2], [3,4,5,6,7], []
        expected = {-3,-2,100,-1,0,1,2,3,4,5,6,7};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }

        // insert causing shift right
        d1.insert(d1.end() - 2, 100);
        // expect - [0,0,0,-3,-2], [100,-1,0,1,2], [3,4,5,100,6], [7,0,0,0,0]
        expected = {-3,-2,100,-1,0,1,2,3,4,5,100,6,7};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }

        //
        // erase
        //

        // erase from front
        d1.erase(d1.begin());
        // expect - [0,0,0,0,-2], [100,-1,0,1,2], [3,4,5,100,6], [7,0,0,0,0]
        expected = {-2,100,-1,0,1,2,3,4,5,100,6,7};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }

        // erase from back
        d1.erase(d1.end() - 1);
        // expect - [0,0,0,0,-2], [100,-1,0,1,2], [3,4,5,100,6], [0,0,0,0,0]
        expected = {-2,100,-1,0,1,2,3,4,5,100,6};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }

        // erase from middle (shift left case)
        d1.erase(d1.begin() + 2);
        // expect - [0,0,0,0,0], [-2,100,0,1,2], [3,4,5,100,6], [0,0,0,0,0]
        expected = {-2,100,0,1,2,3,4,5,100,6};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }

        // erase from middle (shift right case)
        d1.erase(d1.end() - 3);
        // expect - [0,0,0,0,0], [-2,100,0,1,2], [3,4,100,6,0], [0,0,0,0,0]
        expected = {-2,100,0,1,2,3,4,100,6};
        for (size_t i = 0; i < expected.size(); i++) { assert(d1[i] == expected[i]); }
    }

    template <typename T>
    void deque_testIterateHelper(int chunkSize, int n) {
        rack::deque<T> d(chunkSize);
        for (int i = 0; i < n; i++) { d.push_back(T(i)); }

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
            assert(*(d.begin() + i) == T(i));
        }
        for (int i = 1; i <= n; i++) {
            assert(*(d.end() - i) == T(n - i));
        }

        // access / dereference
        it = d.begin();
        assert(*(it + 1) == T(1));
        for (int i = 0; i < n; i++) {
            assert(*(it + i) == T(i));
            assert(it[i] == T(i));
        }

        // looping (mutates elements)
        for (auto &el : d) { el++; }
        for (int i = 0; i < n; i++) { assert(d[i]-- == T(i + 1)); }

        std::for_each(d.begin(), d.end(), [](T& el) { el++; });
        for (int i = 0; i < n; i++) { assert(d[i]-- == T(i + 1)); }

        // sort
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 100);
        for (int i = 0; i < n; ++i) {
            d[i] = T(dist(gen));
        }
        std::sort(d.begin(), d.end());
        assert(std::is_sorted(d.begin(), d.end()));
    }

    void deque_testIterate() {
        int chunkSize, n;

        // run iterator tests for different chunk sizes
        for (int i = 4; i < 16; i++) {
            chunkSize = i * sizeof(int);
            n = chunkSize * 4;

            // primitive type 
            deque_testIterateHelper<int>(chunkSize, n);

            // custom object type
            // deque_testIterateHelper<MyClass>(chunkSize, n);
        }
    }
};
};

int main() {
    // vector tests
    rack::vector_testGeneral();
    rack::vector_testIterate();

    // shared_ptr tests
    rack::shared_ptr_testGeneral();

    // deque tests
    rack::DequeTests dt;
    dt.deque_testPushFrontPushBack();
    dt.deque_testInsertErase();
    dt.deque_testIterate();
    return 0;
}