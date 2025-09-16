#pragma once
#include "shared_ptr.hpp"

namespace rack {
template <class T>
class weak_ptr {
private:
    T* ptr;
    SharedPtrControlBlock* controlBlock;

    template <class U>
    friend class shared_ptr; // allow shared_ptr to access shared_ptr private members

public:
    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////

    weak_ptr() : ptr(nullptr), controlBlock(nullptr) {}

    // copy constructor
    weak_ptr(const weak_ptr& wp)
        : ptr(wp.ptr), controlBlock(wp.controlBlock) 
    {
        if (controlBlock) {
            controlBlock->weakCnt.fetch_add(1, std::memory_order_relaxed);
        }
    }

    // construct from shared_ptr
    weak_ptr(const shared_ptr<T>& sp) 
        : ptr(sp.ptr), controlBlock(sp.controlBlock) 
    {
        if (controlBlock) {
            controlBlock->weakCnt.fetch_add(1, std::memory_order_relaxed);
        }
    }

    // copy-assign from shared_ptr
    weak_ptr& operator=(const shared_ptr<T>& sp) {
        if (controlBlock == sp.controlBlock) {
            return *this;
        }

        // release current ref
        reset();

        // take new one
        ptr = sp.ptr;
        controlBlock = sp.controlBlock;
        if (controlBlock) {
            controlBlock->weakCnt.fetch_add(1, std::memory_order_relaxed);
        }

        return *this;
    }

    ~weak_ptr() {
        reset();
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    void reset() {
        ptr = nullptr;
        if (controlBlock) {
            uint32_t oldWeakCnt = controlBlock->weakCnt.fetch_sub(1, std::memory_order_relaxed);
            uint32_t currStrongCnt = controlBlock->strongCnt.load(std::memory_order_relaxed);
            if (oldWeakCnt == 1 && currStrongCnt == 0) {
                delete controlBlock;
                controlBlock = nullptr;
            }
            
        }
    }

    void swap(weak_ptr<T>& other) noexcept {
        std::swap(ptr, other.ptr);
        std::swap(controlBlock, other.controlBlock);
    }

    //////////////////////////////////////////////////////
    // Observers
    //////////////////////////////////////////////////////

    // Number of strong references (i.e. shared_ptr's) to underlying resource
    long use_count() const noexcept {
        if (controlBlock) {
            return controlBlock->strongCnt.load();
        }
        return 0;
    }

    bool expired() const noexcept {
        return use_count() == 0;
    }

    // Returns shared_ptr to underlying resource
    shared_ptr<T> lock() const noexcept {
        return expired() ? shared_ptr<T>() : shared_ptr<T>(*this);
    }
};
}; // end of 'rack'

