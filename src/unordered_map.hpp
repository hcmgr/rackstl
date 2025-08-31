#include <iostream>
#include <memory>
#include <algorithm>
#include "utils.hpp"

#define DEFAULT_INIT_CAPACITY (1 << 12)     
#define MAX_CAPACITY (1 << 30)

//
// Robin-hood hashing. Power of two table size, to ensure fast modulo. Grow table via doubling.
//

namespace rack {

template <class K, class V, class Alloc = std::allocator<std::pair<const K, V>>>
class unordered_map {
private:
    enum BucketState { EMPTY, OCCUPIED, DELETED };

    struct Bucket {
        std::pair<K, V> kv;
        uint32_t probeDist;
        BucketState state;

        Bucket() : kv(), probeDist(0), state(EMPTY) {}
    };

    Bucket* table;
    size_t _size;
    size_t _capacity;

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
            // too large
            throw std::runtime_error(
                "Requested capacity too large: " + std::to_string(initCapacity) + " (max=" + std::to_string(MAX_CAPACITY) + ")"
            );
        }

        if ((initCapacity & (initCapacity - 1)) != 0) {
            // not divisible by 2
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
    size_t max_size() const { return MAX_CAPACITY; }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////
    void clear() {
        for (size_t i = 0; i < _capacity; i++) {
            Bucket& b = table[i];
            if (b.state == OCCUPIED) {
                utils::allocDestroy(tableAllocator, b.kv);
                b.state = EMPTY;
                b.probeDist = 0;
            } else if (b.state == DELETED) {
                b.state = EMPTY;
                b.probeDist = 0;
            }
        }
    }

    bool insert(const K& key, const V& value) {
        size_t homeIdx = mod(keyHash(key)); 
        size_t currIdx = homeIdx;
        std::pair<K, V> currKv = std::make_pair(key, value);

        //
        // To find a slot, move forward until either:
        //      - find empty/deleted slot, OR;
        //      - find richer slot -> steal (i.e. swap, and continue search with the new element)
        // Available capacity before inserting is guaranteed by the load factor invariant, i.e.
        // (size > LOAD_FACTOR * capacity) => resize.
        //
        while (1) {
            Bucket& bucket = table[currIdx];

            if ((bucket.state == OCCUPIED || bucket.state == DELETED) && 
                mod(homeIdx - currIdx) == 1) {
                // wrapped around to beginning - end search
                return false;
            }

            if (bucket.state == OCCUPIED) {
                if (bucket.probeDist < mod(currIdx - homeIdx)) {
                    // steal
                    std::swap(bucket.kv, currKv);
                    bucket.probeDist = mod(currIdx - homeIdx);
                    homeIdx = mod(keyHash(currKv.first));
                    currIdx = mod(currIdx + 1);
                } else {
                    // advance
                    currIdx = mod(currIdx + 1);
                }
            } else {
                // found empty/deleted slot - greedily use it
                bucket.kv = currKv;
                bucket.probeDist = currIdx - homeIdx;
                bucket.state = OCCUPIED;
                break;
            }
        }
        _size++;
        return true;
    }

    bool erase(const K& key) {

    }

    //////////////////////////////////////////////////////
    // Lookup / accessors
    //////////////////////////////////////////////////////
    V& operator[](const K& key) {

    }

    iterator find(const K& key) {
        size_t homeIdx = mod(keyHash(key));
        size_t currIdx = homeIdx;
        while (1) {
            Bucket& bucket = table[currIdx];

            switch (bucket.state) {
            case OCCUPIED:
                if (bucket.kv.first == key) {
                    return begin();
                }
                currIdx = mod(currIdx + 1);
                break;
            case DELETED:
                currIdx = mod(currIdx + 1);
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
    public:
        Bucket* bucketPtr;
        Bucket* endPtr;

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
        }

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

    //
    // Fast modulo (i.e. equivalent to key % _capacity). 
    // Only works if _capacity is a power of two.
    //
    size_t mod(int idx) {
        return idx & (_capacity - 1);
    }
};
}; // end of 'rack'