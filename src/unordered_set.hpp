#pragma once
#include "unordered_map.hpp"

namespace rack {

//
// unordered_set is just a thin wrapper around our unordered_map. Elements are stored
// as unordered_map keys, where a dummy char is used as the value.
//
template <class K>
class unordered_set {
private:
    rack::unordered_map<K, char> m;

public:
    class iterator;

    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////

    unordered_set() = default;

    // Copy constructor
    unordered_set(const unordered_set& other) 
        : m(other.m) {}

    // Move constructor
    unordered_set(unordered_set&& other)
        : m(std::move(other.m)) {}

    //////////////////////////////////////////////////////
    // Capacity
    //////////////////////////////////////////////////////
    bool empty() const { return m.empty(); }
    size_t size() const { return m.size(); }
    size_t capacity() const { return m.capacity(); }
    size_t max_size() const { return m.max_size(); }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    std::pair<iterator, bool> insert(const K& key) {
        auto [it, inserted] = m.insert({key, 0});
        return {iterator(it), inserted};
    }

    size_t erase(const K& key) { 
        return m.erase(key); 
    }

    iterator erase(iterator it) {
        auto map_iterator = m.erase(it.mapIt);
        return iterator(map_iterator);
    }

    void clear() noexcept { 
        m.clear(); 
    }

    //////////////////////////////////////////////////////
    // Lookup
    //////////////////////////////////////////////////////

    iterator find(const K& key) { 
        return iterator(m.find(key));
    }

    bool contains(const K& key) { 
        return m.find(key) != m.end(); 
    }

    //////////////////////////////////////////////////////
    // Iterator
    //////////////////////////////////////////////////////

    class iterator {
    public:
        using map_iterator = typename rack::unordered_map<K, char>::iterator;
        map_iterator mapIt;

        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = K;
        using pointer           = value_type*;
        using reference         = value_type&;

        iterator() = default;
        iterator(map_iterator i) : mapIt(i) {}

        reference operator*() const { return mapIt->first; }
        pointer operator->() const { return &(mapIt->first); }

        iterator& operator++() { ++mapIt; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++mapIt; return tmp; }

        bool operator==(const iterator& other) const { return mapIt == other.mapIt; }
        bool operator!=(const iterator& other) const { return mapIt != other.mapIt; }

        // internal access if needed
        map_iterator base() const { return mapIt; }
    };

    iterator begin() {
        return iterator(m.begin());
    }

    iterator end() {
        return iterator(m.end());
    }

    //////////////////////////////////////////////////////
    // Hash policy
    //////////////////////////////////////////////////////

    float loadFactor() {
        return m.loadFactor();
    }

    float maxLoadFactor() {
        return m.maxLoadFactor();
    }

    void rehash(size_t n) {
        m.rehash(n);
    }

    void reserve(size_t n) {
        m.reserve(n);
    }

    //////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////
    std::string to_string() 
    {
        return m.to_string();
    }
};

} // namespace rack
