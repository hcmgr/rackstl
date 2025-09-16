#pragma once
#include <atomic>

namespace rack {

// forward declare weak_ptr
template <class T>
class weak_ptr;

//
// Control block is shared by each shared_ptr referencing `ptr`.
// Note that atomics are used for strong and weak count to ensure 
// thread safety.
//
struct SharedPtrControlBlock {

    std::atomic<uint32_t> strongCnt;
    std::atomic<uint32_t> weakCnt;

    SharedPtrControlBlock()
        : strongCnt(0), weakCnt(0) {}
};

template <class T>
class shared_ptr {
private:
    T* ptr;
    SharedPtrControlBlock* controlBlock;

    template <class U>
    friend class weak_ptr; // allow weak_ptr to access shared_ptr private members

public:
    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////

    shared_ptr() {
        ptr = nullptr;
        controlBlock = nullptr;
    }

    shared_ptr(T* p) {
        ptr = p;
        controlBlock = new SharedPtrControlBlock();
        controlBlock->strongCnt.fetch_add(1, std::memory_order_relaxed);
    }

    ~shared_ptr() {
        release();
    }

    // Copy constructor
    shared_ptr(const shared_ptr& other) {
        ptr = other.ptr;
        controlBlock = other.controlBlock;
        controlBlock->strongCnt.fetch_add(1, std::memory_order_relaxed);
    }

    // Move constructor
    shared_ptr(shared_ptr&& other) {
        ptr = other.ptr;
        controlBlock = other.controlBlock;
        other.ptr = nullptr;
        other.controlBlock = nullptr;
    }

    // Construct from weak pointer
    shared_ptr(const weak_ptr<T>& other) 
        : ptr(other.ptr), controlBlock(other.controlBlock)
    {
        if (controlBlock) {
            controlBlock->strongCnt.fetch_add(1, std::memory_order_relaxed);
        }
    }

    // Copy assignment
    shared_ptr<T>& operator=(const shared_ptr<T>& other) {
        if (this != &other) {
            reset();
            ptr = other.ptr;
            controlBlock = other.controlBlock;
            if (controlBlock) { // `other` could be a null shared_ptr (perfectly valid)
                controlBlock->strongCnt.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return *this;
    }

    // Move assignment
    shared_ptr& operator=(shared_ptr&& other) {
        if (this != &other) {
            reset();
            ptr = other.ptr;
            controlBlock = other.controlBlock;
            other.ptr = nullptr;
            other.controlBlock = nullptr;
        }
        return *this;
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    void reset() {
        release();
        ptr = nullptr;
        controlBlock = nullptr;
    }

    void reset(T* newPtr) {
        release();
        if (newPtr) {
            ptr = newPtr;
            controlBlock = new SharedPtrControlBlock();
            controlBlock->strongCnt.fetch_add(1, std::memory_order_relaxed);
        }
    }

    // Swap pointers to managed object with `other`.
    void swap(shared_ptr& other) {
        std::swap(ptr, other.ptr);
        std::swap(controlBlock, other.controlBlock);
    }

    //////////////////////////////////////////////////////
    // Observers
    //////////////////////////////////////////////////////

    T* get() {
        return ptr;
    }

    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }

    uint32_t use_count() {
        if (controlBlock == nullptr) {
            return 0;
        }
        return controlBlock->strongCnt.load(std::memory_order_relaxed);
    }

    bool unique() {
        return controlBlock && controlBlock->strongCnt.load(std::memory_order_relaxed) == 1;
    }

    operator bool() const {
        return ptr != nullptr;
    }

private:
    //
    // Releases ownership of the managed object by:
    //      - decrementing strong refnct AND;
    //      - if applicable, de-allocating underlying memory and/or control block
    //
    void release() {
        // already released
        if (controlBlock == nullptr) { 
            return; 
        }

        uint32_t oldStrongCnt = controlBlock->strongCnt.fetch_sub(1, std::memory_order_relaxed);
        if (oldStrongCnt == 1) {

            // we were last owner - free pointer
            delete ptr;
            ptr = nullptr;

            // also no non-owning references left - free control block
            if (controlBlock->weakCnt.load() == 0) {
                delete controlBlock;
            }
            controlBlock = nullptr;
        }
    }
};

template <class T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
    return shared_ptr<T>(new T(std::forward<Args>(args)...));
}
}; // end of 'rack'
