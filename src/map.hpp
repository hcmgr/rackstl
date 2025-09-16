#pragma once
#include <memory>
#include <algorithm>

#define BLACK   0
#define RED     1

//
// Map uses a red-black tree (RB-tree).
// 
// An RB-tree is a binary search tree (BST) with extra properties.
// These extra properties ensure the tree remains balanced, giving
// us a balanced BST. 
// 
// A map needs:
//      - fast lookup, insert, delete
//      - ordering
//
// The 'balanced' property ensures O(logn) traversal => O(logn) lookup,
// insert and delete. An in-order traversal also gives us our nodes in order.
// So, an RB-tree is particularly amenable for a map.
//
// These extra properties are:
//      1. Each node black or red
//      2. Root of node always black
//      3. No consecutive red nodes allowed
//      4. All paths from node to leaves has same number of null nodes
// 
// Why this ensures balance:
//      (3) and (4) combine to ensure that root path lengths differ by
//      at most 2x, preserving balance. Specifically, it ensures our
//      height is always O(log2n).
//
//      Calculation:
//          (4) ensures all paths from root have black height bh. With (3),
//          the minimum path length is bh, and the max is 2*bh.
//          Full binary tree of height bh has at least 2^bh - 1 nodes.
//          Thus:
//              n >= 2^bh - 1 ==> 
//              bh <= log2(n + 1) ==>
//              h <= 2 * log2(n + 1) ==>
//              h is O(log2n).
//
namespace rack {
template <class K, class V, class Alloc = std::allocator<std::pair<const K, V>>>
class map {
private:
    struct RBNode {
        std::pair<K, V> kv;
        RBNode* left;
        RBNode* right;
        bool colour;
    };
};
}; // end of 'rack'
