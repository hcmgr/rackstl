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
    //////////////////////////////////////////////////////
    // Construtors
    //////////////////////////////////////////////////////

    vector() 
        : buffPtr(nullptr), mCapacity(0), mSize(0) {
        // do nothing - allocation of `mBuff` occurs on first element added
    }

    ~vector() {

    }

    // Constructs container of `n` copies of `val`.
    vector(uint32_t n, T val) 
        : mCapacity(n), mSize(n) {
        buffPtr = alloc.allocate(mCapacity);
        for (int i = 0; i < n; i++) {
            buffPtr[i] = val;
        }
    }

    // Copy constructor (i.e. MyClass b = a, constructing b by copying a)
    vector(const vector& other) {

    }

    // Move constructor (i.e. MyClass b = std::move(a), constructing b by moving a)
    vector(vector&& other) {

    }

    // Copy assignment
    vector& operator=(const vector& other) {

    }

    // Move assignment 
    vector& operator=(vector&& other) noexcept {

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

    template <typename... Args>
    void emplace_back(Args&&... args) {
        // first element added - allocate mBuff of capacity 1
        if (mCapacity == 0) {
            assert(mSize == 0 && buffPtr == nullptr);
            mCapacity = 1;
            buffPtr = alloc.allocate(mCapacity);
        }

        // enough space for val
        if (mSize < mCapacity) {
            utils::allocConstruct(alloc, buffPtr + mSize, std::forward<Args>(args)...);
            mSize++;
            return;
        }

        // 
        // not enough space for val - grow the container
        //

        // create new buffer of size 2n
        T* newBuffPtr = alloc.allocate(2 * mCapacity);

        // copy n elements from old buffer into new buffer
        for (int i = 0; i < mSize; i++) {
            utils::allocConstruct(alloc, newBuffPtr + i, buffPtr[i]);
        }
        
        // add val
        utils::allocConstruct(alloc, newBuffPtr + mSize, std::forward<Args>(args)...);

        //
        // Teardown old buffer.
        //
        for (int i = 0; i < mSize; ++i) {
            utils::allocDestroy(alloc, buffPtr + i);
        }
        alloc.deallocate(buffPtr, mCapacity);

        // update new buffer
        buffPtr = newBuffPtr;
        mCapacity = 2 * mCapacity;
        mSize++;
    }

    //
    // Adds copy of `val` to the end of the container.
    // If capacity is reached, the container grows via a doubling strategy.
    // 
    // Note: uses emplace_back() under the hood
    //
    void push_back(const T& val) {
        emplace_back(val);
    }

    // Inserts copy of `val` before `pos`
    void insert(T val, uint32_t pos) {

    }

    // Erases element at `pos` from container
    void erase(uint32_t pos) {

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

    std::string to_string() 
    {
        std::ostringstream oss;
        oss << "[";
        for (uint32_t i = 0; i < mSize; ++i) 
        {
            oss << buffPtr[i];
            if (i != mSize - 1) {
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
};

}; // end of 'rack'