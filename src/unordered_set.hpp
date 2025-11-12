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

    bool empty() const noexcept { 
        return m.empty(); 
    }

    size_t size() const noexcept { 
        return m.size(); 
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    std::pair<iterator, bool> insert(const K& key) {
        auto [it, inserted] = m.insert({key, 0});
        return {it, inserted};
    }

    size_t erase(const K& key) { 
        return m.erase(key); 
    }

    void clear() noexcept { 
        m.clear(); 
    }

    //////////////////////////////////////////////////////
    // Lookup
    //////////////////////////////////////////////////////

    iterator find(const K& key) { 
        return m.find(key); 
    }

    bool contains(const K& key) const { 
        return m.find(key) != m.end(); 
    }

    //////////////////////////////////////////////////////
    // Iterator
    //////////////////////////////////////////////////////

    class iterator {
    private:
        using map_iterator = typename rack::unordered_map<K, char>::iterator;
        map_iterator it;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = K;
        using pointer           = const K*;
        using reference         = const K&;

        iterator() = default;
        iterator(map_iterator i) : it(i) {}

        reference operator*() const { return it->first; }
        pointer operator->() const { return &(it->first); }

        iterator& operator++() { ++it; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++it; return tmp; }

        bool operator==(const iterator& other) const { return it == other.it; }
        bool operator!=(const iterator& other) const { return it != other.it; }

        // internal access if needed
        map_iterator base() const { return it; }
    };


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
};

} // namespace rack
