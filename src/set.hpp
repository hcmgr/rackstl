#include "map.hpp"

namespace rack {

//
// set<> is just a thin wrapper around our map<>. Elements are stored
// as map<> keys, where a dummy char is used as the value.
//
template <class K>
class set {
private:
    rack::map<K, char> m;

public:
    class iterator;

    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////

    set() = default;

    // copy constructor
    set(const set& other) {

    }

    // move constructor
    set (set&& other) {

    }

    ~set() {

    }

    //////////////////////////////////////////////////////
    // Capacity
    //////////////////////////////////////////////////////
    bool empty() { return m.empty(); }
    size_t size() { return m.size(); }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////
    void clear() { m.clear(); }

    std::pair<iterator, bool> insert(K& k) {
        auto [it, inserted] = m.insert({key, 0});
        return {iterator(it), inserted};
    }

    //////////////////////////////////////////////////////
    // Iterator
    //////////////////////////////////////////////////////

    class iterator {
    private:
        using map_iterator = typename rack::map<K, char>::iterator;
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

    iterator begin() {
        return iterator(m.begin());
    }

    iterator end() {
        return iterator(m.end());
    }
};
}; // end of 'rack'