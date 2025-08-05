#include <sstream>
#include <cassert>
#include <iostream>

namespace rack {

template <class T>
class vector {
private:
    T* _buff;
    uint32_t _capacity;
    uint32_t _size;

public:

    //////////////////////////////////////////////////////
    // Construtors
    //////////////////////////////////////////////////////

    vector() 
        : _buff(nullptr), _capacity(0), _size(0) {
        // do nothing - allocation of `_buff` occurs on first element added
    }

    ~vector() {

    }

    //
    // Constructs container of `n` copies of `val`.
    //
    // NOTE: For now, container initialised with capacity `n` (i.e. immediately full capacity).
    //       Room for optimisation. Potentially initialise it as 2n?
    //
    vector(uint32_t n, T val) 
        : _capacity(n), _size(n) {
        _buff = static_cast<T*>(::operator new(sizeof(T) * _capacity));
        for (int i = 0; i < n; i++) {
            _buff[i] = val;
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
    T& operator [](uint32_t i) {
        if (i < 0 || i >= _size) {
            throw std::runtime_error(
                "Index out of bounds error: " +
                std::string("index=") + std::to_string(i) + ", size=" + std::to_string(_size)
            );
        }
        return _buff[i];
    }

    // First element of container
    T& front() { 
        return _buff[0]; 
    }

    // Last element of container
    T& back() {
        return _buff[_size - 1]; 
    }

    // Accesses pointer to underlying container
    T* data() { 
        return _buff; 
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    //
    // Adds copy of `val` to the end of the container.
    // If capacity is reached, the container grows via a doubling strategy.
    //
    void push_back(const T& val) {
        // first element added - allocate _buff of capacity 1
        if (_capacity == 0) {
            assert(_size == 0 && _buff == nullptr);
            _capacity++;
            _buff = static_cast<T*>(::operator new(sizeof(T) * _capacity));
        }

        // enough space for val
        if (_size < _capacity) {
            new (&_buff[_size]) T(val); // note use of 'placement new' operator
            _size++;
            return;
        }

        // 
        // not enough space for val - grow the container
        //

        // create new buffer of size 2n
        T* newBuffPtr = static_cast<T*>(::operator new(sizeof(T) * 2 * _capacity));

        // copy n elements from old buffer into new buffer
        for (int i = 0; i < _size; i++) {
            new (&newBuffPtr[i]) T(_buff[i]);
        }
        
        // add val
        new (&newBuffPtr[_size]) T(val);
        _size++;

        //
        // Teardown old buffer.
        //
        // Note that `::operator delete(_buff)` only de-allocates the memory buffer.
        // We must also also destruct each object of the old array.
        //
        for (int i = 0; i < _size; ++i) {
            _buff[i].~T();
        }
        ::operator delete(_buff);

        // point _buff to new buffer
        _buff = newBuffPtr;
        _capacity = 2 * _capacity;
    }

    // Constructs element in place using `args` and performs 'push_back' operation
    template <typename... Args>
    void emplace_back(Args&&... args) {

    }

    // Inserts copy of `val` before `pos`
    void insert(T val, uint32_t pos) {

    }

    // Erases element at `pos` from container
    void erase(uint32_t pos) {

    }

    // Clears the contents of the container
    void clear() {

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
    // Capacity
    //////////////////////////////////////////////////////

    bool empty() {
        return _size == 0;
    }

    uint32_t size() {
        return _size;
    }

    uint32_t capacity() {
        return _capacity;
    }

    // Reserve capacity ahead of time.
    void reserve(uint32_t capacity);

    //////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////

    std::string to_string() 
    {
        std::ostringstream oss;
        oss << "[ ";
        for (uint32_t i = 0; i < _size; ++i) 
        {
            oss << _buff[i];
            if (i != _size - 1) {
                oss << ", ";
            }
        }
        oss << " ]\n";
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
        return iterator(_buff);
    }

    iterator end() {
        return iterator(_buff + _size);
    }
};

}; // end of 'rack'