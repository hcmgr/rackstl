#pragma once
namespace rack {

template <class T>
class unique_ptr {
private:
    T* ptr;

public:

    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////

    unique_ptr()
        : ptr(nullptr) {}

    unique_ptr(T* p)
        : ptr(p) {}

    // copy constructor - not allowed
    unique_ptr(const unique_ptr& other) = delete;

    // copy assign - not allowed
    unique_ptr& operator=(const unique_ptr& other) = delete;

    // move constructor
    unique_ptr(unique_ptr&& other) {
        this->ptr = other.ptr;
        other.ptr = nullptr;
    }

    // move assign
    unique_ptr& operator=(unique_ptr&& other) {
        if (this != &other) {
            reset();
            this->ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~unique_ptr() {
        delete ptr;
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    T* release() {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    void reset() {
        if (ptr != nullptr) {
            delete ptr;
            ptr = nullptr;
        }
    }

    void reset(T* newPtr) {
        reset();
        ptr = newPtr;
    }

    void swap(unique_ptr& other) {
        std::swap(ptr, other.ptr);
    }


    //////////////////////////////////////////////////////
    // Observers
    //////////////////////////////////////////////////////
    T* get() const {
        return ptr;
    }

    operator bool() const {
        return ptr != nullptr;
    }

    //////////////////////////////////////////////////////
    // Dereference 
    //////////////////////////////////////////////////////
    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }

    //////////////////////////////////////////////////////
    // Other operators
    //////////////////////////////////////////////////////
    bool operator==(const unique_ptr& other) const { return ptr == other.ptr; }
    bool operator!=(const unique_ptr& other) const { return ptr != other.ptr; }
    bool operator<(const unique_ptr& other) const { return ptr < other.ptr; }
    bool operator<=(const unique_ptr& other) const { return ptr <=  other.ptr; }
    bool operator>(const unique_ptr& other) const { return ptr > other.ptr; }
    bool operator>=(const unique_ptr& other) const { return ptr >= other.ptr; }
};

template <class T, class...Args>
unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}; // end 'rack'