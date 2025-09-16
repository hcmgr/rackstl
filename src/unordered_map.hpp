#pragma once
#include <iostream>
#include <memory>
#include <algorithm>

#include "utils.hpp"

#define DEFAULT_INIT_CAPACITY (1 << 12)     
#define MAX_CAPACITY (1 << 30)

//
// Unordered_map uses robin-hood hashing.
//
// Robin-hood hashing is a hash table strategy that improves on linear probing. 
// It does so by minimising each key's 'probe distance', i.e. the distance a key
// is from its 'home' bucket (hash(key) % N).
//
// On a hash collision, linear probing is performed as normal to find an available 
// bucket. However, if, on your probing journey, you find a key with a lower probe
// distance: swap, and continue your search with the swapped key (i.e. steal from 
// rich, give to the poor, hence: robin-hood).
//
// We also maintain a power-of-2 table size. This is so we can use the 'fast modulo'
// operation:
//      i % N <==> i & (N - 1), where N == 2^k (for some k)
// which only works if N is a power-of-2. Naturally, this means we grow the table
// via a doubling strategy.
//
namespace rack {

template <class K, class V, class Alloc = std::allocator<std::pair<const K, V>>>
class unordered_map {
private:
    enum BucketState { EMPTY, OCCUPIED, DELETED };

    struct Bucket {
        std::pair<K, V> kv;
        size_t probeDist;
        BucketState state;

        Bucket() : kv(), probeDist(0), state(EMPTY) {}
    };

    Bucket* table;
    size_t _size;
    size_t _capacity;
    float _maxLoadFactor = 0.75f;

    std::allocator<Bucket> tableAllocator;

    std::hash<K> keyHash;

public:
    class iterator;

    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////
    unordered_map() 
        : unordered_map(DEFAULT_INIT_CAPACITY) {
    }

    unordered_map(size_t initCapacity) {
        if (initCapacity > MAX_CAPACITY) {
            // capacity too large
            throw std::runtime_error(
                "Requested capacity too large: " + std::to_string(initCapacity) + " (max=" + std::to_string(MAX_CAPACITY) + ")"
            );
        }

        if ((initCapacity & (initCapacity - 1)) != 0) {
            // capacity not divisible by 2
            throw std::runtime_error(
                "Custom capacity must be divisible by 2"
            );
        }

        _capacity = initCapacity;
        _size = 0;
        tableAllocator = std::allocator<Bucket>();
        
        table = tableAllocator.allocate(_capacity);
        for (size_t i = 0; i < _capacity; i++) {
            utils::allocConstruct(tableAllocator, table + i);
        }

        keyHash = std::hash<K>{}; 
    }

    unordered_map(std::initializer_list<std::pair<const K, V>> init) {

    }

    //
    // Copy constructor.
    // 
    // Creates a new table of equal capacity, and copies elements from 'other' into it.
    // Table allocator also copied.
    //
    unordered_map(const unordered_map& other) {
        _size = other._size;
        _capacity = other._capacity;

        std::allocator<Bucket> tableAllocator = 
            std::allocator_traits<Alloc>::select_on_container_copy_construction(other.tableAllocator);

        table = tableAllocator.allocate(_capacity);
        for (size_t i = 0; i < _capacity; i++) {
            // only copy occupied states - if empty or deleted, construct empty bucket
            if (other.table[i].state == OCCUPIED) {
                utils::allocConstruct(tableAllocator, table + i, other.table[i]);
            } else {
                utils::allocConstruct(tableAllocator, table + i);
            }
        }
    }

    // move constructor
    unordered_map(unordered_map&& other) {

    }

    ~unordered_map() {
        for (size_t i = 0; i < _capacity; i++) {
            utils::allocDestroy(tableAllocator, table + i);
        }
        tableAllocator.deallocate(table, _capacity);
    }

    //////////////////////////////////////////////////////
    // Capacity
    //////////////////////////////////////////////////////
    bool empty() const { return _size == 0; }
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    size_t max_size() const { return MAX_CAPACITY; }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////
    void clear() {
        for (size_t i = 0; i < _capacity; i++) {
            Bucket& b = table[i];
            if (b.state == OCCUPIED) {
                utils::allocDestroy(tableAllocator, &b.kv);
                b.state = EMPTY;
                b.probeDist = 0;
                _size--;
            } else if (b.state == DELETED) {
                b.state = EMPTY;
                b.probeDist = 0;
            }
        }
        assert(_size == 0);
    }

    std::pair<iterator, bool> insert(std::pair<K, V> kv) {
        if (loadFactor() >= _maxLoadFactor) {
            // max load factor exceeded - resize with table of 2x capacity
            rehash(_capacity * 2);
        }

        size_t homeIdx = mod(keyHash(kv.first), _capacity); 
        size_t currIdx = homeIdx;
        std::pair<K, V> currKv = kv;

        //
        // To find a slot, move forward until either:
        //      - find empty/deleted slot, OR;
        //      - find richer slot -> steal (i.e. swap, and continue search with the new element)
        // Available capacity before inserting is guaranteed by the load factor invariant, i.e.
        // (size > LOAD_FACTOR * capacity) => resize.
        //
        while (1) {
            Bucket& bucket = table[currIdx];

            switch (bucket.state) {
            case OCCUPIED:
                if (bucket.kv.first == currKv.first) {
                    // key alread exists - return early
                    std::cout << "key already exists" << "\n";
                    return {iterator(table + currIdx, table + _capacity), true};
                }

                if (bucket.probeDist < mod(currIdx - homeIdx, _capacity)) {
                    // steal
                    std::swap(bucket.kv, currKv);
                    bucket.probeDist = mod(currIdx - homeIdx, _capacity);
                    homeIdx = mod(keyHash(currKv.first), _capacity);
                    currIdx = mod(currIdx + 1, _capacity);
                } else {
                    // advance
                    currIdx = mod(currIdx + 1, _capacity);
                }
                break;
            case EMPTY:
            case DELETED:
                // found empty/deleted slot - greedily use it
                bucket.kv = currKv;
                bucket.probeDist = currIdx - homeIdx;
                bucket.state = OCCUPIED;
                _size++;
                return {iterator(table + currIdx, table + _capacity), true};
            default:
                throw std::runtime_error("Unexpected bucket state - " + std::to_string(bucket.state));
            }

            if (currIdx == homeIdx) {
                // wrapped around to beginning without finding free slot - end search
                // note: should never happen due to the load factor invariant
                return {end(), false};
            }
        }
    }

    iterator erase(iterator pos) {
        if (pos < begin() || pos >= end()) {
            throw std::runtime_error("iterator invalid");
        }

        iterator next = pos + 1;

        Bucket& bucket = *(pos.bucketPtr);
        utils::allocDestroy(tableAllocator, &(bucket.kv));
        bucket.state = EMPTY;

        return next;
    }

    iterator erase(const K& key) {
        return erase(find(key));
    }

    //////////////////////////////////////////////////////
    // Lookup / accessors
    //////////////////////////////////////////////////////

    V& at(const K& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("Key not in map");
        }
        return it->second;
    }

    //
    // Returns reference to value of `key`, performing an insertion if key doesn't 
    // already exist.
    //
    V& operator[](const K& key) {
        auto [it, _] = insert({key, V{}});
        return it->second;
    }

    V& operator[](K&& key) {
        auto [it, _] = insert({std::move(key), V{}});  
        return it->second;
    }

    iterator find(const K& key) {
        size_t homeIdx = mod(keyHash(key), _capacity);
        size_t currIdx = homeIdx;
        while (1) {
            Bucket& bucket = table[currIdx];

            switch (bucket.state) {
            case OCCUPIED:
                if (bucket.kv.first == key) {
                    return iterator(table + currIdx, table + _capacity);
                }
                currIdx = mod(currIdx + 1, _capacity);
                break;
            case DELETED:
                currIdx = mod(currIdx + 1, _capacity);
                break;
            case EMPTY:
                return end();
            default:
                throw std::runtime_error("Unexpected bucket state - " + std::to_string(bucket.state));
            }

            if (currIdx == homeIdx) {
                // wrapped around to beginning without finding - end search
                return end();
            }
        }
    }

    bool contains(const K& key) {
        return find(key) != end();
    }

    //////////////////////////////////////////////////////
    // Bucket interface / iterator
    //////////////////////////////////////////////////////

    //
    // 'iterator' is a simple wrapper around the table pointer.
    // Like std::unordered_map, our iterator satisfies ForwardIterator, which
    // only requires *it, ++it / it++ and == / !=.
    //
    class iterator {
    private:
        Bucket* bucketPtr;
        Bucket* endPtr;
    public:

        // typedefs - necessary for other STL functions to use this (e.g. std::sort)
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::pair<K, V>;
        using pointer           = value_type*;
        using reference         = value_type&;

        // constructors
        iterator() : bucketPtr(nullptr), endPtr(nullptr) {}
        iterator(Bucket* p, Bucket* end) : bucketPtr(p), endPtr(end) {}
        iterator(const iterator& other) : bucketPtr(other.bucketPtr) {}
        ~iterator() {}

        // copy assign
        iterator& operator=(const iterator& other) {
            bucketPtr = other.bucketPtr;
            return *this;
        }

        //
        // ForwardIterator operators
        //

        // reference
        value_type operator*() const { return bucketPtr->kv; }
        value_type* operator->() const { return &(bucketPtr->kv); }

        // equality
        bool operator==(const iterator& other) const { return bucketPtr == other.bucketPtr; }
        bool operator!=(const iterator& other) const { return !(*this == other); }

        // pre-inc
        iterator& operator++() {
            ++bucketPtr;
            while (bucketPtr != endPtr && bucketPtr->state != OCCUPIED) {
                ++bucketPtr;
            }
            return *this;
        } 

        // post-inc
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        //
        // Other operators - implemented for internal use
        //

        iterator& operator+=(size_t i) {
            while (i-- > 0) {
                ++(*this);
            }
            return *this;
        }

        iterator operator+(size_t i) {
            iterator newIt = *this;
            newIt += i;
            return newIt;
        }
    };

    iterator begin() {
        Bucket* startPtr = table;
        Bucket* endPtr = table + _capacity;
        while (startPtr != endPtr && startPtr->state != OCCUPIED) {
            startPtr++;
        }
        return iterator(startPtr, endPtr);
    }

    iterator end() {
        return iterator(table + _capacity, table + _capacity);
    }

    //////////////////////////////////////////////////////
    // Hash policy
    //////////////////////////////////////////////////////

    float loadFactor() {
        return static_cast<float>(_size) / static_cast<float>(_capacity);
    }

    float maxLoadFactor() {
        return _maxLoadFactor;
    }

    //
    // Changes number of buckets, with a lower-bound of `n`.
    // If `n` doesn't satisfy the load factor invariant, `n` is bumped
    // up to 2*_size (i.e. load factor of 0.5)
    //
    void rehash(size_t n) {
        if (!loadFactorSatisfied(n)) {
            n = 2 * _size;
        }

        Bucket* oldTable = table;
        size_t oldSize = _size;
        size_t oldCapacity = _capacity;

        // initialise new table
        table = tableAllocator.allocate(n);
        _size = 0;
        _capacity = n;
        for (size_t i = 0; i < _capacity; i++) {
            utils::allocConstruct(tableAllocator, table + i);
        }

        // copy buckets from previous table and free old table resources
        for (size_t i = 0; i < oldCapacity; i++) {
            Bucket& bucket = oldTable[i];
            if (bucket.state == OCCUPIED) {
                insert({bucket.kv.first, bucket.kv.second});
                utils::allocDestroy(tableAllocator, &bucket.kv);
            }
        }
        tableAllocator.deallocate(oldTable, oldCapacity);
    }

    void reserve(size_t n) {

    }

    //////////////////////////////////////////////////////
    // Observers
    //////////////////////////////////////////////////////

    //////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////

    std::string to_string() 
    {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < _capacity; ++i) 
        {
            Bucket& bucket = table[i];
            if (bucket.state == OCCUPIED) {
                oss << table[i].kv.first
                    << ":" 
                    << std::to_string(table[i].kv.second);
            } else if (bucket.state == DELETED) {
                oss << "X";
            } else {
                oss << "_";
            }
            if (i != _capacity - 1) {
                oss << ",";
            }
        }
        oss << "]";
        return oss.str();
    }

    //////////////////////////////////////////////////////
    // Helpers 
    //////////////////////////////////////////////////////

private:
    //
    // Fast modulo (i.e. equivalent to key % _capacity). 
    // Only works if _capacity is a power of two.
    //
    size_t mod(int idx, size_t N) {
        return idx & (N - 1);
    }

    // Returns true if bucket count `n` satisfies the load factor invariant, false otherwise.
    bool loadFactorSatisfied(size_t n) {
        return _maxLoadFactor * static_cast<float>(n) > static_cast<float>(_size);
    }
};
}; // end of 'rack'