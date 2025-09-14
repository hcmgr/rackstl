#pragma once
#include "unordered_map.hpp"

namespace rack {

//
// Our unordered_set is just a shallow wrapper around unordered_map, using 
// char as a dummy value.
//
template <class K, class Alloc = std::allocator<K>>
class unordered_set {
private:
    using MapType = unordered_map<K, char, std::allocator<std::pair<const K, char>>>;
    MapType map;

public:
    class iterator {
        typename MapType::iterator it;
    public:
        iterator() = default;
        iterator(typename MapType::iterator i) : it(i) {}
        K& operator*() const { return (*it).first; }
        K* operator->() const { return &(it->first); }
        iterator& operator++() { ++it; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++it; return tmp; }
        bool operator==(const iterator& other) const { return it == other.it; }
        bool operator!=(const iterator& other) const { return it != other.it; }
    };

    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////
    unordered_set() = default;
    unordered_set(size_t initCapacity) : map(initCapacity) {}
    unordered_set(std::initializer_list<K> init) {
        for (const auto& key : init) {
            insert(key);
        }
    }

    //////////////////////////////////////////////////////
    // Capacity
    //////////////////////////////////////////////////////
    bool empty() const { return map.empty(); }
    size_t size() const { return map.size(); }
    size_t capacity() const { return map.capacity(); }
    size_t max_size() const { return map.max_size(); }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////
    std::pair<iterator, bool> insert(const K& key) {
        auto [it, inserted] = map.insert({key, 0});
        return {iterator(it), inserted};
    }

    void clear() { map.clear(); }

    size_t erase(const K& key) {
        auto it = map.find(key);
        if (it != map.end()) {
            map.erase(it);
            return 1;
        }
        return 0;
    }

    //////////////////////////////////////////////////////
    // Lookup
    //////////////////////////////////////////////////////
    iterator find(const K& key) { return iterator(map.find(key)); }
    bool contains(const K& key) { return map.contains(key); }

    //////////////////////////////////////////////////////
    // Iterators
    //////////////////////////////////////////////////////
    iterator begin() { return iterator(map.begin()); }
    iterator end() { return iterator(map.end()); }

    //////////////////////////////////////////////////////
    // Hash policy
    //////////////////////////////////////////////////////
    float loadFactor() { return map.loadFactor(); }
    float maxLoadFactor() { return map.maxLoadFactor(); }
    void rehash(size_t n) { map.rehash(n); }
    void reserve(size_t n) { map.reserve(n); }

    //////////////////////////////////////////////////////
    // Debugging
    //////////////////////////////////////////////////////
    std::string to_string() { return map.to_string(); }
};

} // namespace rack