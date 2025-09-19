#pragma once
#include <sstream>
#include <cassert>
#include <iostream>

#include "utils.hpp"

namespace rack {

template <class T>
class vector {
private:
    T* buffPtr;
    uint32_t mCapacity;
    uint32_t mSize;

    std::allocator<T> alloc;
    
public:
    class iterator; // forward-declare iterator

    //////////////////////////////////////////////////////
    // Construtors
    //////////////////////////////////////////////////////

    vector() 
        : buffPtr(nullptr), mCapacity(0), mSize(0) 
    {
        // do nothing - lazily allocate buffer
    }

    // Constructs container of `n` copies of `val`.
    vector(uint32_t n, T val) 
        : mCapacity(n), mSize(n) 
    {
        buffPtr = alloc.allocate(mCapacity);
        for (int i = 0; i < n; i++) {
            buffPtr[i] = val;
        }
    }

    ~vector() {
        destroy();
    }

    // Copy constructor
    vector(const vector& other)  {
        copyOther(other);
    }

    // Move constructor 
    vector(vector&& other) {
        moveOther();
    }

    // Copy assign
    vector& operator=(const vector& other) {
        destroy();
        copyOther(other);
    }

    // Move assign
    vector& operator=(vector&& other) noexcept {
        destroy();
        moveOther(other);
    }

    //////////////////////////////////////////////////////
    // Accessors
    //////////////////////////////////////////////////////

    // [] operator override
    T& operator[](uint32_t i) {
        if (i < 0 || i >= mSize) {
            throw std::runtime_error(
                "Index out of bounds error: " +
                std::string("index=") + std::to_string(i) + ", size=" + std::to_string(mSize)
            );
        }
        return buffPtr[i];
    }

    // First element of container
    T& front() { 
        return buffPtr[0]; 
    }

    // Last element of container
    T& back() {
        return buffPtr[mSize - 1]; 
    }

    // Accesses pointer to underlying container
    T* data() { 
        return buffPtr; 
    }

    //////////////////////////////////////////////////////
    // Capacity
    //////////////////////////////////////////////////////

    bool empty() {
        return mSize == 0;
    }

    uint32_t size() {
        return mSize;
    }

    uint32_t capacity() {
        return mCapacity;
    }

    // Reserve capacity ahead of time
    void reserve(uint32_t capacity);

    // Frees un-used capacity of the container
    void shrink_to_fit();

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    //
    // Constructs a T from `args` in-place and adds it to the back
    // of the container. 
    //
    template <typename... Args>
    void emplace_back(Args&&... args) {
        // lazily allocate buffer on first insertion
        if (mCapacity == 0) {
            mCapacity = 1;
            buffPtr = alloc.allocate(mCapacity);
        }

        // grow buffer if out of capacity
        if (mSize == mCapacity) {
            grow();
        }

        utils::allocConstruct(alloc, buffPtr + mSize, std::forward<Args>(args)...);
        mSize++;
    }

    //
    // Adds a copy of `val` to the back of the container.
    //
    void push_back(const T& val) {
        emplace_back(val);
    }

    //
    // Inserts a copy of `val` before `pos`
    //
    iterator insert(iterator pos, T val) {
        // lazily allocate buffer on first insertion
        if (mCapacity == 0) {
            mCapacity = 1;
            buffPtr = alloc.allocate(mCapacity);
            pos = iterator(buffPtr);
        }

        // invalid range
        if (pos < begin() || pos > end()) {
            return end();
        }

        int32_t posOff = pos.ptr - buffPtr;

        // grow buffer if out of capacity
        if (mSize == mCapacity) {
            grow();
        }

        // shift all right-elements one position to right
        if (mSize > 1) {
            int32_t startOff = posOff;
            int32_t endOff = mSize - 1;
            if (endOff - startOff >= 0) {
                shiftRight(startOff, endOff);
            }
        }

        utils::allocConstruct(alloc, buffPtr + posOff, val);
        mSize++;
        return iterator(buffPtr + posOff);
    }

    // Erases element at `pos` from container
    iterator erase(iterator pos) {
        // invalid range
        if (pos < begin() || pos >= end()) {
            return end();
        }

        // erase element
        int32_t posOff = pos.ptr - buffPtr;
        utils::allocDestroy(alloc, buffPtr + posOff);

        // shift all right-elements one position to left
        if (mSize > 1) {
            int32_t startOff = posOff + 1;
            int32_t endOff = mSize - 1;
            if (endOff - startOff >= 0) {
                shiftLeft(startOff, endOff);
            }
            utils::allocDestroy(alloc, buffPtr + endOff);
        }

        mSize--;
        return iterator(buffPtr + posOff);
    }

    // Clears the contents of the container (capacity unchanged)
    void clear() {
        for (int i = 0; i < mSize; i++) {
            utils::allocDestroy(alloc, buffPtr);
        }
        mSize = 0;
    }

    //
    // Resizes container to `count` elements.
    //  
    // If `count` == size, does nothing.
    // If `count` < size, container reduced to first `count` elements.
    // If `count` > size, additional copies of T() are appended.
    //
    void resize(uint32_t count) {

    }

    //////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////

    std::string toString() 
    {
        std::ostringstream oss;
        oss << "[";
        for (uint32_t i = 0; i < mCapacity; ++i) 
        {
            if (buffPtr[i]) {
                oss << buffPtr[i];
            } else {
                oss << "_";
            }

            if (i != mCapacity - 1) {
                oss << ",";
            }
        }
        oss << "]\n";
        return oss.str();
    }

    //////////////////////////////////////////////////////
    // Iterators
    //////////////////////////////////////////////////////

    class iterator {
    public:
        T* ptr;

        // typedefs - necessary for other STL functions to use this (e.g. std::sort)
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        // constructors
        iterator() : ptr(nullptr) {}
        explicit iterator(T* p) : ptr(p) {}

        // dereference
        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }

        // comparison
        bool operator==(const iterator& other) const { return ptr == other.ptr; }
        bool operator!=(const iterator& other) const { return ptr != other.ptr; }
        bool operator<(const iterator& other) const { return ptr < other.ptr; }
        bool operator<=(const iterator& other) const { return ptr <= other.ptr; }
        bool operator>(const iterator& other) const { return ptr > other.ptr; }
        bool operator>=(const iterator& other) const { return ptr >= other.ptr; }

        // arithmetic
        iterator operator+(uint32_t i) const { return iterator(ptr + i); }
        iterator operator-(uint32_t i) const { return iterator(ptr - i); }
        int operator-(const iterator& other) const { return ptr - other.ptr; }

        iterator& operator++() { ++ptr; return *this; } // pre-increment
        iterator operator++(int) { iterator tmp = *this; ++ptr; return tmp; } // post-increment
        iterator& operator--() { --ptr; return *this; }
        iterator operator--(int) { iterator tmp = *this; --ptr; return tmp; }

        iterator& operator+=(uint32_t i) { ptr += i; return *this; }
        iterator& operator-=(uint32_t i) { ptr -= i; return *this; }

        // index
        T& operator[](uint32_t i) const { return *(ptr + i); }
    };


    iterator begin() {
        return iterator(buffPtr);
    }

    iterator end() {
        return iterator(buffPtr + mSize);
    }

    //////////////////////////////////////////////////////
    // Helpers
    //////////////////////////////////////////////////////

private:
    void copyOther(const vector& other) {
        mCapacity = other.mCapacity;
        mSize = other.mSize;
        buffPtr = alloc.allocate(mCapacity);
        for (int i = 0; i < mSize; i++) {
            utils::allocConstruct(alloc, buffPtr + i, other.buffPtr + i);
        }
    }

    void moveOther(vector&& other) {
        mCapacity = other.mCapacity;
        mSize = other.mSize;
        buffPtr = other.buffPtr;

        other.buffPtr = nullptr;
        other.mCapacity = 0;
        other.mSize = 0;
    }

    void destroy() {
        if (buffPtr) {
            for (size_t i = 0; i < mSize; ++i) {
                utils::allocDestroy(alloc, buffPtr + i);
            }
            alloc.deallocate(buffPtr, mCapacity);
        }
        buffPtr = nullptr;
        mSize = 0;
        mCapacity = 0;
    }

    // 
    // Shift all elements in range [startIdx, endIdx] (inclusive) one position to the left.
    //
    void shiftLeft(uint32_t startIdx, uint32_t endIdx) {
        assert(startIdx <= endIdx);

        uint32_t currIdx = startIdx - 1;
        while (currIdx != endIdx) {
            buffPtr[currIdx] = std::move(buffPtr[currIdx + 1]);
            currIdx++;
        }
    }

    // 
    // Shift all elements in range [startIdx, endIdx] (inclusive) one position to the right.
    // Note that we assume there is available capacity.
    //
    void shiftRight(uint32_t startIdx, uint32_t endIdx) {
        assert(startIdx <= endIdx);

        uint32_t currIdx = endIdx + 1;
        while (currIdx != startIdx) {
            buffPtr[currIdx] = std::move(buffPtr[currIdx - 1]);
            currIdx--;
        }
    }

    //
    // Grow the buffer 2x in capacity
    //
    void grow() {
        // create new buffer of 2x capacity
        T* newBuffPtr = alloc.allocate(2 * mCapacity);

        // copy n elements from old buffer into new buffer
        for (int i = 0; i < mSize; i++) {
            utils::allocConstruct(alloc, newBuffPtr + i, buffPtr[i]);
        }

        // teardown old buffer
        for (int i = 0; i < mSize; ++i) {
            utils::allocDestroy(alloc, buffPtr + i);
        }
        alloc.deallocate(buffPtr, mCapacity);

        // update new buffer
        buffPtr = newBuffPtr;
        mCapacity = 2 * mCapacity;
    }
};

}; // end of 'rack'