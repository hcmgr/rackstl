#include <iostream>
#include <cassert>
#include <cmath>
#include <random> 
#include <algorithm>
#include <memory>

#include "vector.hpp"
#include "shared_ptr.hpp"

////////////////////////////////////////
// Tests
////////////////////////////////////////

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

void vector_testPushBack() {

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

    rack::vector<int>::iterator it1 = vec1.begin();
    for (int i = 0; i < n; i++) {
        it1++;
    }
    assert(it1 == vec1.end());

    it1 = vec1.begin();
    it1 += n;
    assert(it1 == vec1.end());

    assert(n == std::distance(vec1.begin(), vec1.end()));

    // dereference
    rack::vector<MyClass> vec2;
    vec2.push_back(MyClass(0));
    rack::vector<MyClass>::iterator it2 = vec2.begin();
    assert((*it2).val == vec2[0].val);
    assert(it2->val == vec2[0].val);

    // index
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

void shared_ptr_test() {
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

int main() {
    shared_ptr_test();
    return 0;
}