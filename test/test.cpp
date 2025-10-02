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
#include <map>
#include <thread>

#include "vector.hpp"
#include "shared_ptr.hpp"
#include "unique_ptr.hpp"
#include "weak_ptr.hpp"
#include "deque.hpp"
#include "unordered_map.hpp"
#include "map.hpp"
#include "bst.hpp"

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

TEST(vector_test, pushBackEmplaceBack) {

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

TEST(vector_test, insertErase) {
    //
    // empty vector insert
    //
    rack::vector<int> vec;
    EXPECT_TRUE(vec.empty());
    vec.insert(vec.begin(), 42);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 42);

    //
    // single element erase
    //
    vec.erase(vec.begin());
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);

    //
    // empty erase
    //
    vec.erase(vec.begin());
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);

    //
    // insert multiple at end
    //
    for (int i = 0; i < 5; i++) {
        vec.push_back(i + 10);  // [10,11,12,13,14]
    }
    EXPECT_EQ(vec.size(), 5);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(vec[i], i + 10);
    }

    //
    // insert multiple at beginning
    //
    vec.insert(vec.begin(), 99); // [99,10,11,12,13,14]
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[0], 99);

    //
    // insert in middle
    //
    vec.insert(vec.begin() + 3, 77); // [99,10,11,77,12,13,14]
    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec[3], 77);

    //
    // erase first element
    //
    vec.erase(vec.begin()); // [10,11,77,12,13,14]
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[0], 10);

    //
    // erase middle element
    //
    vec.erase(vec.begin() + 2); // remove 77 -> [10,11,12,13,14]
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[2], 12);

    //
    // erase last element
    //
    vec.erase(vec.end() - 1); // remove 14 -> [10,11,12,13]
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[3], 13);

    //
    // final content check
    //
    std::vector<int> expected = {10, 11, 12, 13};
    for (size_t i = 0; i < expected.size(); i++) {
        EXPECT_EQ(vec[i], expected[i]);
    }
}

TEST(vector_test, reserveShrinkResize) {
    int n = 100;

    //
    // reserve
    //
    rack::vector<int> vec1;
    vec1.reserve(n);
    EXPECT_EQ(vec1.capacity(), n);
    for (int i = 0; i < n; i++) {
        vec1.push_back(i);
    }
    EXPECT_EQ(vec1.size(), n);
    EXPECT_EQ(vec1.capacity(), n);
    vec1.push_back(n);
    EXPECT_EQ(vec1.size(), n + 1);
    EXPECT_GT(vec1.capacity(), n + 1);
    n += 1;
    
    //
    // shrink_to_fit
    //
    vec1.shrink_to_fit();
    EXPECT_EQ(vec1.size(), n);
    EXPECT_EQ(vec1.capacity(), n);
    
    //
    // resize growing
    //
    int growSize = 2 * n;
    vec1.resize(growSize, 420);
    EXPECT_EQ(vec1.size(), growSize);
    EXPECT_GE(vec1.capacity(), growSize);
    for (int i = 0; i < n; i++) {
        EXPECT_EQ(vec1[i], i);
    }
    for (int i = n; i < growSize; i++) {
        EXPECT_EQ(vec1[i], 420);
    }

    //
    // resize shrinking
    //
    vec1.resize(n);
    EXPECT_EQ(vec1.size(), n);
    for (int i = 0; i < n; i++) {
        EXPECT_EQ(vec1[i], i);
    }
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
}

////////////////////////////////////////
// BST tests 
////////////////////////////////////////

TEST(bst_test, general) {
    rack::BSTTree<int, int>* tree = new rack::BSTTree<int, int>();

    //
    // tree 1 (sideways view):
    //
    //          8
    //      7   
    //          6
    //  5       
    //          4
    //      3
    //          2
    //              1
    //

    //
    // insert and find
    //
    tree->insert({5, 5}); // level 1
    tree->insert({3, 3}); // level 2
    tree->insert({7, 7}); 
    tree->insert({2, 2}); // level 3
    tree->insert({4, 4});
    tree->insert({6, 6});
    tree->insert({8, 8});
    tree->insert({1, 1}); // level 4
    // std::cout << tree->toString();
    rack::vector<int> v = tree->inOrderVec();

    // validate in-order traversal
    ASSERT_EQ(v, rack::vector<int>({1,2,3,4,5,6,7,8}));

    // validate tree structure
    BSTNode<int, int>* node;
    node = tree->find(2);
    ASSERT_EQ(node->kv.first, 2);
    ASSERT_EQ(node->left->kv.first, 1);
    ASSERT_EQ(node->right, nullptr);
    node = tree->find(4);
    ASSERT_EQ(node->kv.first, 4);
    ASSERT_EQ(node->left, nullptr);
    ASSERT_EQ(node->right, nullptr);
    node = tree->find(6);
    ASSERT_EQ(node->kv.first, 6);
    ASSERT_EQ(node->left, nullptr);
    ASSERT_EQ(node->right, nullptr);
    node = tree->find(8);
    ASSERT_EQ(node->kv.first, 8);
    ASSERT_EQ(node->left, nullptr);
    ASSERT_EQ(node->right, nullptr);
    node = tree->find(3);
    ASSERT_EQ(node->kv.first, 3);
    ASSERT_EQ(node->left->kv.first, 2);
    ASSERT_EQ(node->right->kv.first, 4);
    node = tree->find(7);
    ASSERT_EQ(node->kv.first, 7);
    ASSERT_EQ(node->left->kv.first, 6);
    ASSERT_EQ(node->right->kv.first, 8);
    node = tree->find(5);
    ASSERT_EQ(node->kv.first, 5);
    ASSERT_EQ(node->left->kv.first, 3);
    ASSERT_EQ(node->right->kv.first, 7);
    node = tree->find(0);
    ASSERT_EQ(node, nullptr);
    node = tree->find(9);
    ASSERT_EQ(node, nullptr);

    // 
    // erase
    //
    bool res;

    // erase 8 (child)
    res = tree->erase(8);
    node = tree->find(7);

    //
    //          
    //      7   
    //          6
    //  5       
    //          4
    //      3
    //          2
    //              1
    //
    EXPECT_EQ(res, true);
    EXPECT_EQ(node->kv.first, 7);
    EXPECT_EQ(node->left->kv.first, 6);
    EXPECT_EQ(node->right, nullptr);

    // erase 7 (node with one-child)
    res = tree->erase(7);

    //
    //          
    //      6   
    //
    //  5       
    //          4
    //      3
    //          2
    //              1
    //
    node = tree->find(5);
    EXPECT_EQ(res, true);
    EXPECT_EQ(node->kv.first, 5);
    EXPECT_EQ(node->left->kv.first, 3);
    EXPECT_EQ(node->right->kv.first, 6);

    // erase 3 (node with two-children)
    res = tree->erase(3);
    
    //          
    //      6   
    //
    //  5       
    //          
    //      4
    //          2
    //              1
    //
    node = tree->find(4);
    EXPECT_EQ(res, true);
    EXPECT_EQ(node->kv.first, 4);
    EXPECT_EQ(node->left->kv.first, 2);
    EXPECT_EQ(node->right, nullptr);
    node = tree->find(5);
    EXPECT_EQ(node->kv.first, 5);
    EXPECT_EQ(node->left->kv.first, 4);
    EXPECT_EQ(node->right->kv.first, 6);

    //
    // clear
    //
    tree->clear();
    ASSERT_EQ(tree->size(), 0);
    ASSERT_EQ(tree->root, nullptr);
}

TEST(bst_test, leftRightRotations) {
    rack::BSTTree<int, int>* tree = new rack::BSTTree<int, int>();

    tree->insert({5,5});
    tree->insert({7,7});
    tree->insert({3,3});
    //
    //      7
    //  5
    //      3
    //
    ASSERT_EQ(tree->root->kv.first, 5);
    ASSERT_EQ(tree->root->left->kv.first, 3);
    ASSERT_EQ(tree->root->right->kv.first, 7);

    tree->rotateLeft(tree->root);
    //
    //  7
    //      5
    //          3
    //
    ASSERT_EQ(tree->root->kv.first, 7);
    ASSERT_EQ(tree->root->left->kv.first, 5);
    ASSERT_EQ(tree->root->left->left->kv.first, 3);

    tree->rotateLeft(tree->root);
    //
    //  7
    //      5
    //          3
    //
    ASSERT_EQ(tree->root->kv.first, 7);
    ASSERT_EQ(tree->root->left->kv.first, 5);
    ASSERT_EQ(tree->root->left->left->kv.first, 3);

    tree->rotateRight(tree->root);
    tree->rotateRight(tree->root);
    //
    //          7
    //      5
    //  3
    //
    ASSERT_EQ(tree->root->kv.first, 3);
    ASSERT_EQ(tree->root->right->kv.first, 5);
    ASSERT_EQ(tree->root->right->right->kv.first, 7);

    tree->rotateRight(tree->root);
    //
    //          7
    //      5
    //  3
    //
    ASSERT_EQ(tree->root->kv.first, 3);
    ASSERT_EQ(tree->root->right->kv.first, 5);
    ASSERT_EQ(tree->root->right->right->kv.first, 7);

    tree->rotateLeft(tree->root);
    tree->insert({2,2});
    tree->insert({4,4});
    //
    //      7
    //  5
    //          4
    //      3
    //          2
    //
    ASSERT_EQ(tree->root->kv.first, 5);
    ASSERT_EQ(tree->root->left->kv.first, 3);
    ASSERT_EQ(tree->root->left->left->kv.first, 2);
    ASSERT_EQ(tree->root->left->right->kv.first, 4);

    tree->rotateLeft(tree->root->left);
    //
    //      7
    //  5
    //      4
    //          3
    //              2
    //
    ASSERT_EQ(tree->root->kv.first, 5);
    ASSERT_EQ(tree->root->left->kv.first, 4);
    ASSERT_EQ(tree->root->left->left->kv.first, 3);
    ASSERT_EQ(tree->root->left->left->left->kv.first, 2);

    tree->rotateRight(tree->root->left);
    tree->rotateRight(tree->root->left);
    ASSERT_EQ(tree->root->kv.first, 5);
    ASSERT_EQ(tree->root->left->kv.first, 2);
    ASSERT_EQ(tree->root->left->right->kv.first, 3);
    ASSERT_EQ(tree->root->left->right->right->kv.first, 4);
}

TEST(bst_test, comboRotations) {
    rack::BSTTree<int, int>* tree = new rack::BSTTree<int, int>();
    tree->insert({6,6});
    tree->insert({3,3});
    tree->insert({8,8});
    tree->insert({1,1});
    tree->insert({4,4});
    
    //
    //      8
    //  6   
    //          4
    //      3
    //          1
    //
    ASSERT_EQ(tree->root->kv.first, 6);
    ASSERT_EQ(tree->root->left->kv.first, 3);
    ASSERT_EQ(tree->root->right->kv.first, 8);
    ASSERT_EQ(tree->root->left->left->kv.first, 1);
    ASSERT_EQ(tree->root->left->right->kv.first, 4);

    /////////////////////////////////////////
    // left-left
    /////////////////////////////////////////

    tree->rotateLeftLeft(tree->root);
    tree->insert({2,2});
    //
    //          8
    //      6
    //          4
    //  3
    //          2
    //      1
    //
    ASSERT_EQ(tree->root->kv.first, 3);
    ASSERT_EQ(tree->root->left->kv.first, 1);
    ASSERT_EQ(tree->root->left->right->kv.first, 2);
    ASSERT_EQ(tree->root->right->kv.first, 6);
    ASSERT_EQ(tree->root->right->left->kv.first, 4);
    ASSERT_EQ(tree->root->right->right->kv.first, 8);

    /////////////////////////////////////////
    // left-right
    /////////////////////////////////////////

    tree->rotateLeftRight(tree->root->left, tree->root);
    //
    //                                      8
    //          8                       6
    //      6                               4
    //          4                   3
    //  3               ->      2
    //      2                       1
    //          1
    //
    ASSERT_EQ(tree->root->kv.first, 2);
    ASSERT_EQ(tree->root->left->kv.first, 1);
    ASSERT_EQ(tree->root->right->kv.first, 3);
    ASSERT_EQ(tree->root->right->right->kv.first, 6);
    ASSERT_EQ(tree->root->right->right->left->kv.first, 4);
    ASSERT_EQ(tree->root->right->right->right->kv.first, 8);

    /////////////////////////////////////////
    // right-right
    /////////////////////////////////////////

    tree->rotateRightRight(tree->root->right);
    //
    //              8
    //          6
    //                  4
    //              3
    //      2
    //          1
    //
    ASSERT_EQ(tree->root->kv.first, 2);
    ASSERT_EQ(tree->root->left->kv.first, 1);
    ASSERT_EQ(tree->root->right->kv.first, 6);
    ASSERT_EQ(tree->root->right->left->kv.first, 3);
    ASSERT_EQ(tree->root->right->right->kv.first, 8);
    ASSERT_EQ(tree->root->right->left->right->kv.first, 4);

    /////////////////////////////////////////
    // right-left
    /////////////////////////////////////////

    tree->rotateRightLeft(tree->root->right, tree->root);
    //
    //                  8
    //              6                       8
    //                  4               6
    //          3                           4
    //      2                 ->    3
    //          1                       2
    //                                      1
    //
    ASSERT_EQ(tree->root->kv.first, 3);
    ASSERT_EQ(tree->root->left->kv.first, 2);
    ASSERT_EQ(tree->root->left->left->kv.first, 1);
    ASSERT_EQ(tree->root->right->kv.first, 6);
    ASSERT_EQ(tree->root->right->left->kv.first, 4);
    ASSERT_EQ(tree->root->right->right->kv.first, 8);
}

/////////////////////////////////////////
// RBTree tests
/////////////////////////////////////////

TEST(rb_test, general) {
    rack::RBTree<int, int>* rbTree = new rack::RBTree<int, int>();

    // no recolour needed
    rbTree->insert({5, 5});
    rbTree->insert({7, 7});
    rbTree->insert({3, 3});
    
    //  
    //      7(R)
    //  5(B)
    //      3(R)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 5 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->left->kv.first == 3 && rbTree->root->left->colour == RED);
    ASSERT_TRUE(rbTree->root->right->kv.first == 7 && rbTree->root->right->colour == RED);

    // recolour - red-uncle
    rbTree->insert({8, 8});
    
    //
    //          8(R)
    //      7(B)    
    //  5(B)
    //      3(B)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 5 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->left->kv.first == 3 && rbTree->root->left->colour == BLACK);
    ASSERT_TRUE(rbTree->root->right->kv.first == 7 && rbTree->root->right->colour == BLACK);
    ASSERT_TRUE(rbTree->root->right->right->kv.first == 8 && rbTree->root->right->right->colour == RED);

    // no recolour needed
    rbTree->insert({4, 4});
    
    //
    //          8(R)
    //      7(B)    
    //  5(B)
    //          4(R)
    //      3(B)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 5 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->left->kv.first == 3 && rbTree->root->left->colour == BLACK);
    ASSERT_TRUE(rbTree->root->right->kv.first == 7 && rbTree->root->right->colour == BLACK);
    ASSERT_TRUE(rbTree->root->right->right->kv.first == 8 && rbTree->root->right->right->colour == RED);
    ASSERT_TRUE(rbTree->root->left->right->kv.first == 4 && rbTree->root->left->right->colour == RED);
    ASSERT_EQ(rbTree->findMin()->kv.first, 3);
    ASSERT_EQ(rbTree->findMax()->kv.first, 8);

    // reset
    rbTree->clear();
    rbTree->insert({5,5});
    rbTree->insert({4,4});
    //
    //  5(B)
    //      4(R)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 5 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->left->kv.first == 4 && rbTree->root->left->colour == RED);

    // insert 1, then recolour - black-uncle, left-left case
    rbTree->insert({1,1});
    //
    //                              5(R)
    //  5(B)          ->        4(B)
    //      4(R)                    1(R)
    //          3(R)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 4 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->left->kv.first == 1 && rbTree->root->left->colour == RED);
    ASSERT_TRUE(rbTree->root->right->kv.first == 5 && rbTree->root->right->colour == RED);

    rbTree->clear();
    rbTree->insert({5,5});
    rbTree->insert({3,3});
    //
    //  5(B)
    //      3(R)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 5 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->left->kv.first == 3 && rbTree->root->left->colour == RED);

    // insert 3, then recolour - black-uncle, left-right case
    rbTree->insert({4,4});
    //
    //                            5(R)
    //  5(B)            ->    4(B)
    //          4(R)              3(R)
    //      3(R)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 4 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->left->kv.first == 3 && rbTree->root->left->colour == RED);
    ASSERT_TRUE(rbTree->root->right->kv.first == 5 && rbTree->root->right->colour == RED);

    rbTree->clear();
    rbTree->insert({5,5});
    rbTree->insert({6,6});
    //
    //      6(R)
    //  5(B)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 5 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->right->kv.first == 6 && rbTree->root->right->colour == RED);

    // insert 7, then recolour - black-uncle, right-right case
    rbTree->insert({7,7});
    //
    //          7(R)
    //      6(R)                    7(R)
    //  5(B)            ->      6(B)
    //                              5(R)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 6 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->left->kv.first == 5 && rbTree->root->left->colour == RED);
    ASSERT_TRUE(rbTree->root->right->kv.first == 7 && rbTree->root->right->colour == RED);

    rbTree->clear();
    rbTree->insert({5,5});
    rbTree->insert({7,7});
    //
    //      7(R)
    //  5(B)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 5 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->right->kv.first == 7 && rbTree->root->right->colour == RED);

    // insert 6, then recolour - black-uncle, right-left case
    rbTree->insert({6,6});
    //
    //      7(R)
    //          6(R)              7(R)
    //  5(B)            ->    6(B)
    //                            5(R)
    //
    ASSERT_TRUE(rbTree->root->kv.first == 6 && rbTree->root->colour == BLACK);
    ASSERT_TRUE(rbTree->root->left->kv.first == 5 && rbTree->root->left->colour == RED);
    ASSERT_TRUE(rbTree->root->right->kv.first == 7 && rbTree->root->right->colour == RED);
    ASSERT_EQ(rbTree->findMin()->kv.first, 5);
    ASSERT_EQ(rbTree->findMax()->kv.first, 7);
}

void insertManual(rack::RBNode<int, int>** node, std::pair<int, int> kv, bool colour) {
    *node = new rack::RBNode<int, int>(kv, colour);
}

void freeManual(rack::RBNode<int, int>** node) {
    delete *node;
    *node = nullptr;
}

TEST(rb_test, rb_properties_fine_grain_insert) {
    //
    // Here, we verify our rb properties hold with with careful, fine-grain inserts.
    // Note that manually insert (see insertManual() above) to avoid our colour fixing code,
    // i.e. we want intentionally bad trees so we can verify property checking.
    //
    rack::RBTree<int, int>* rbTree = new rack::RBTree<int, int>();
    insertManual(&rbTree->root, {5,5}, BLACK);
    //
    //  5(B)            (valid)
    //
    ASSERT_TRUE(rbTree->checkRbTree());

    insertManual(&rbTree->root->left, {2,2}, RED);
    //
    //  5(B)          
    //      2(R)        (valid)
    //
    ASSERT_TRUE(rbTree->checkRbTree());

    insertManual(&rbTree->root->left->left, {1,1}, RED);
    //
    //  5(B)
    //      2(R)        (invalid - double red)
    //          1(R)
    //
    ASSERT_FALSE(rbTree->checkRbTree());
    freeManual(&rbTree->root->left->left);

    insertManual(&rbTree->root->right, {6,6}, RED);
    //
    //      6(R)
    //  5(B)            (valid)
    //      2(R)        
    //
    insertManual(&rbTree->root->right->right, {7,7}, BLACK);
    insertManual(&rbTree->root->right->right->right, {8,8}, RED);
    //
    //              8(R)
    //          7(B)
    //      6(R)
    //  5(B)            (invalid - un-equal black heights)
    //      2(R)        
    //
    ASSERT_FALSE(rbTree->checkRbTree());
}

TEST(rb_test, rb_properties_mass_insert) {
    //
    // Here, we verify our rb properties hold with mass insertions.
    //

    rack::RBTree<int, int>* rbTree = new rack::RBTree<int, int>();

    int N = 100;
    bool valid;
    for (int i = 0; i < N; i++) {
        rbTree->insert({i, i});
        valid = rbTree->checkRbTree();
        ASSERT_TRUE(valid);
        if (!valid) {
            std::cout << "\n\n\n\n" << rbTree->toString();
        }
    }

    for (int i = 2*N; i >= N; i--) {
        rbTree->insert({i, i});
        valid = rbTree->checkRbTree();
        ASSERT_TRUE(valid);
        if (!valid) {
            std::cout << "\n\n\n\n" << rbTree->toString();
        }
    }
}

TEST(rb_test, in_order_successor) {
    using Node = rack::RBNode<int, int>;
    using RBTree = rack::RBTree<int, int>;
    rack::RBTree<int, int>* tree = new rack::RBTree<int, int>();
    Node* node;

    tree->insert({3,3});
    tree->insert({2,2});
    //
    //  3   
    //      2
    //
    node = RBTree::inorderSuccessor(tree->root);
    ASSERT_EQ(node, tree->sentinel);
    node = RBTree::inorderSuccessor(tree->root->left);
    ASSERT_EQ(node->kv.first, 3);

    tree->insert({1,1});
    tree->insert({5,5});
    tree->insert({4,4});
    //  
    //                  5:R
    //          4:B
    //                  3:R
    //  2:B
    //          1:B
    //
    node = RBTree::inorderSuccessor(tree->root);
    ASSERT_EQ(node->kv.first, 3);
    node = RBTree::inorderSuccessor(tree->root->left);
    ASSERT_EQ(node->kv.first, 2);
    node = RBTree::inorderSuccessor(tree->root->right);
    ASSERT_EQ(node->kv.first, 5);
    node = RBTree::inorderSuccessor(tree->root->right->left);
    ASSERT_EQ(node->kv.first, 4);
    node = RBTree::inorderSuccessor(tree->root->right->right);
    ASSERT_EQ(node, tree->sentinel);
}

TEST(rb_test, in_order_predecessor) {
    using Node = rack::RBNode<int, int>;
    using RBTree = rack::RBTree<int, int>;
    rack::RBTree<int, int>* tree = new rack::RBTree<int, int>();
    Node* node;

    tree->insert({3,3});
    tree->insert({4,4});
    //
    //      4
    //  3   
    //
    node = RBTree::inorderPredecessor(tree->root);
    ASSERT_EQ(node, tree->sentinel);
    node = RBTree::inorderPredecessor(tree->root->right);
    ASSERT_EQ(node->kv.first, 3);

    tree->insert({5,5});
    tree->insert({1,1});
    tree->insert({2,2});
    //  
    //          5:B
    //  4:B
    //                  3:R
    //          2:B 
    //                  1:R
    //
    node = RBTree::inorderPredecessor(tree->root);
    ASSERT_EQ(node->kv.first, 3);
    node = RBTree::inorderPredecessor(tree->root->left);
    ASSERT_EQ(node->kv.first, 1);
    node = RBTree::inorderPredecessor(tree->root->left->left);
    ASSERT_EQ(node, tree->sentinel);
    node = RBTree::inorderPredecessor(tree->root->left->right);
    ASSERT_EQ(node->kv.first, 2);
    node = RBTree::inorderPredecessor(tree->root->right);
    ASSERT_EQ(node->kv.first, 4);
}

TEST(rb_test, min_max_sentinel) {
    rack::RBTree<int, int>* rbTree = new rack::RBTree<int, int>();

    int N = 10;
    for (int i = 0; i < N; i++) {
        rbTree->insert({i, i});
    }
    ASSERT_EQ(rbTree->findMin()->kv.first, 0);
    ASSERT_EQ(rbTree->findMax()->kv.first, N - 1);
    ASSERT_EQ(rbTree->root->parent, rbTree->sentinel);
    ASSERT_EQ(rbTree->sentinel->parent, rbTree->root);
    ASSERT_EQ(rbTree->sentinel->left, rbTree->findMin());
    ASSERT_EQ(rbTree->sentinel->right, rbTree->findMax());
}

/////////////////////////////////////////
// map tests
/////////////////////////////////////////

TEST(map_test, basic_operations) {
    map<int, std::string> m;

    ASSERT_TRUE(m.empty());
    ASSERT_EQ(m.size(), 0);

    // insert
    m.insert({1, "one"});
    m.insert({2, "two"});
    ASSERT_FALSE(m.empty());
    ASSERT_EQ(m.size(), 2);

    // operator[]
    m[3] = "three";

    ASSERT_EQ(m.size(), 3);
    ASSERT_EQ(m[3], "three");

    // find
    auto it = m.find(2);
    ASSERT_NE(it, m.end());
    ASSERT_EQ(it->second, "two");

    // clear
    m.clear();
    ASSERT_TRUE(m.empty());
    ASSERT_EQ(m.size(), 0);
}

TEST(map_test, iterate) {
    map<int, int> m;
    ASSERT_EQ(m.begin(), m.end());

    int N = 20;
    for (int i = 0; i < N; i++) {
        m.insert({i, 2*i});
    }

    ASSERT_TRUE(m.begin() != m.end());
    ASSERT_TRUE(m.begin() < m.end());
    
    // forward iteration
    auto it = m.begin();
    for (int i = 0; i < N; i++) {
        ASSERT_EQ(it->first, i);
        ASSERT_EQ(it->second, 2*i);
        it++;
    }
    ASSERT_EQ(it, m.end());

    // backward iteration from end()
    it = m.end();
    for (int i = N - 1; i >= 0; i--) {
        --it;
        ASSERT_EQ(it->first, i);
        ASSERT_EQ(it->second, 2*i);
    }
    ASSERT_EQ(it, m.begin());

    // iterator equality checks
    auto it1 = m.begin();
    auto it2 = m.begin();
    ASSERT_TRUE(it1 == it2);
    ++it2;
    ASSERT_TRUE(it1 != it2);

    // ordering checks
    ASSERT_TRUE(it1 < it2);
    ASSERT_TRUE(it2 > it1);
    ASSERT_TRUE(it1 < m.end());
    ASSERT_FALSE(m.end() < it1);
}
}; // end 'rack'