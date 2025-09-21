#pragma once
#include <memory>
#include <algorithm>
#include <vector>

#define BLACK   0
#define RED     1

namespace rack {

template <class K, class V>
struct BSTNode {
    std::pair<K, V> kv;
    BSTNode* parent;
    BSTNode* left;
    BSTNode* right;

    BSTNode(
        const std::pair<K, V>& kvArg, 
        BSTNode* parentArg, 
        BSTNode* leftArg, 
        BSTNode* rightArg)
        : kv(kvArg), parent(parentArg), left(leftArg), right(rightArg) {}
};

template <class K, class V>
class BSTTree {
public:

    using Node = BSTNode<K, V>;

    Node* root;
    uint32_t mSize;

    BSTTree() 
        : root(nullptr), mSize(0) {}
    
    // 
    // Returns pointer to newly inserted node, or existing node if it already exists.
    //
    Node* insert(std::pair<K, V> kv) {
        if (mSize == 0) {
            root = new Node(kv, nullptr, nullptr, nullptr);
            mSize++;
            return root;
        }

        //
        // Search tree until either:
        //      - find key, OR;
        //      - find free slot to insert
        //
        Node* currNode = root;
        bool inserted = false;
        while (true) {
            if (kv.first == currNode->kv.first) {
                // found key
                return currNode;
            }

            if (kv.first < currNode->kv.first) {
                // left 
                if (currNode->left == nullptr) {
                    currNode->left = new Node(kv, currNode, nullptr, nullptr);
                    mSize++;
                    inserted = true;
                }
                currNode = currNode->left;

            } else {
                // right
                if (currNode->right == nullptr) {
                    currNode->right = new Node(kv, currNode, nullptr, nullptr);
                    mSize++;
                    inserted = true;
                }
                currNode = currNode->right;
            }

            if (inserted) {
                break;
            }
        }

        // at this point, we've inserted, and currNode points to the new node
        return currNode;
    }

    bool erase(const K& key) {
        Node* node = find(key);
        if (node == nullptr) {
            return false;
        }

        if (node->left && node->right) {
            // Two children 
            //
            // Strategy:
            //      Swap value with in-order successor, then delete in-order successor.
            //      In-order successor is guaranteed left-most node in right subtree,
            //      so either a leaf or node with one-child (i.e. always a simpler delete).
            //
            Node* succ = inorderSuccessor(node);
            std::swap(node->kv, succ->kv);
            node = succ;
        }

        if (node->left) {
            // left child - directly connect parent and left child
            updateParentRef(node, node->left);
            delete node;
            mSize--;
            return true;
        }

        if (node->right) {
            // right child - directly connect parent and right child
            updateParentRef(node, node->right);
            delete node;
            mSize--;
            return true;
        }
        
        if (node->left == nullptr && node->right == nullptr) {
            // leaf node - just remove it
            updateParentRef(node, nullptr);
            delete node;
            return true;
        }

        assert(false); // never get here
    }

    Node* find(const K& key) {
        if (mSize == 0) {
            return nullptr;
        }

        Node* currNode = root;
        while (true) {
            if (currNode == nullptr) {
                return nullptr;
            }

            if (key == currNode->kv.first) {
                // found key
                return currNode;
            }

            if (key < currNode->kv.first) {
                // left
                currNode = currNode->left;
            } else {
                // right
                currNode = currNode->right;
            }
        }
    }

    void clear() {
        //
        // Post-order traversal - free-ing node at each point.
        //
        // Note we can swap this out for an iterative traversal if deep recursion 
        // issues arise.
        //
        clearPostOrder(root);
        root = nullptr;
        mSize = 0;
    }

    uint32_t size() {
        return mSize;
    }

    rack::vector<K> inOrderVec() {
        rack::vector<K> vec;
        inOrderVecHelper(root, vec);
        return vec;
    }

    //
    // Returns in-order successor of `node`
    //
    Node* inorderSuccessor(Node* target) {
        // case 1: right subtree exists
        if (target->right != nullptr) {
            Node* curr = target->right;
            while (curr->left != nullptr) {
                curr = curr->left;
            }
            return curr;
        }

        // case 2: no right subtree
        Node* succ = nullptr;
        Node* curr = root;
        while (curr != nullptr) {
            if (target->kv.first < curr->kv.first) {
                succ = curr;        // this could be successor
                curr = curr->left;
            } else if (target->kv.first > curr->kv.first) {
                curr = curr->right;
            } else {
                break;
            }
        }
        return succ;
    }

    // Returns side-ways view of BST
    std::string toString() {
        std::ostringstream oss;
        toStringHelper(root, oss, 0);
        return oss.str();
    }

private:
    //
    // Updates parent of `oldNode` to point to `newNode` instead
    //
    void updateParentRef(Node* oldNode, Node* newNode) {
        Node* parent = oldNode->parent;
        if (parent->left == oldNode) {
            parent->left = newNode;
        } else if (parent->right == oldNode) {
            parent->right = newNode;
        }
    }



    void clearPostOrder(Node* currNode) {
        if (currNode == nullptr) {
            return;
        }

        clearPostOrder(currNode->left);
        clearPostOrder(currNode->right);
        delete currNode;
    }

    void inOrderVecHelper(Node* node, rack::vector<K>& vec) {
        if (node == nullptr) {
            return;
        }

        inOrderVecHelper(node->left, vec);
        vec.push_back(node->kv.first);
        inOrderVecHelper(node->right, vec);
    }

    void toStringHelper(Node* node, std::ostringstream& oss, int depth) const {
        if (!node) return;

        // right subtree first (so it prints on top when visualized sideways)
        toStringHelper(node->right, oss, depth + 1);

        // current node (indented according to `depth`)
        oss << std::string(depth * 4, ' ')   // 4 spaces per depth level
            << "(" << node->kv.first << ":" << node->kv.second << ")\n";

        // left subtree
        toStringHelper(node->left, oss, depth + 1);
    }
};

template <class K, class V, class Alloc = std::allocator<std::pair<const K, V>>>
class map {
private:
    BSTTree<K, V>* tree;

public:
    class iterator; // forward-declare iterator

    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////
    map() {
        tree = new BSTTree<K, V>();
    }

    ~map() {
    }

    //////////////////////////////////////////////////////
    // Accessors
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

    //////////////////////////////////////////////////////
    // Capacity
    //////////////////////////////////////////////////////
    bool empty() {
    }

    uint32_t size() {
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////
    void clear();


    void insert(std::pair<K, V> kv);

    template <typename... Args>
    void emplace(Args&&... args);

    iterator erase(iterator pos);

    //////////////////////////////////////////////////////
    // Lookup
    //////////////////////////////////////////////////////
    uint32_t count(const K& key) const;

    iterator find(const K& key) {

    }

    bool contains(const K& key);
    iterator lower_bound(const K& key);
    iterator upper_bound(const K& key);

    //////////////////////////////////////////////////////
    // Observors
    //////////////////////////////////////////////////////

    // key_comp
    // value_comp

    //////////////////////////////////////////////////////
    // Iterator
    //////////////////////////////////////////////////////
    class iterator {

    };

    iterator begin();
    iterator end();

    //////////////////////////////////////////////////////
    // Helpers
    //////////////////////////////////////////////////////
};

//
// Map uses a red-black tree (RB-tree).
// 
// A RB-tree is a binary search tree (BST) with extra properties.
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
//      Intuition:
//          (4) ensures all paths from root have same black height, bh.
//          Then, because of (3): paths are minimum bh, and maximum 2*bh.
//          This leaves paths balanced (specifically, ensures height is always O(logn)).
//
//      Calculation:
//          Min path length bh, max 2*bh.
//          Full binary tree of height bh has at least 2^bh - 1 nodes.
//          Thus:
//              n >= 2^bh - 1 ==> 
//              bh <= log2(n + 1) ==>
//              h <= 2 * log2(n + 1) ==>
//              h is O(log2n).
//

// template <class K, class V>
// struct RBNode {
//     std::pair<K, V> kv;
//     RBNode* parent;
//     RBNode* left;
//     RBNode* right;
//     bool colour;

//     RBNode(
//         const std::pair<K, V>& kvArg, 
//         RBNode* parentArg, 
//         RBNode* leftArg, 
//         RBNode* rightArg, 
//         bool colourArg)
//         : kv(kvArg), parent(parentArg), left(leftArg), right(rightArg), colour(colourArg) {}
// };

// template <class K, class V>
// class RBTree {
//     RBNode* root;
//     uint32_t mSize;
    
//     RBTree() 
//         : root(nullptr), mSize(0) {}
    
//     // 
//     // Returns pointer to newly inserted node, or existing node if it already exists.
//     //
//     RBNode* insert(std::pair<K, V> kv) {
//         if (mSize == 0) {
//             tree->root = new RBNode(kv, nullptr, nullptr, BLACK);
//             mSize++;
//             return;
//         }

//         //
//         // Search tree until either:
//         //      - find key, OR;
//         //      - find free slot to insert
//         //
//         RBNode* currNode = tree->root;
//         bool inserted = false;
//         while (true) {
//             if (kv.first == currNode->kv.first) {
//                 // found key
//                 return currNode;
//             }

//             if (kv.first < currNode->kv.first) {
//                 // left 
//                 if (currNode->left == nullptr) {
//                     currNode->left = new RBNode(kv, nullptr, nullptr, RED);
//                     mSize++;
//                     inserted = true;
//                 }
//                 currNode = currNode->left;

//             } else {
//                 // right
//                 if (currNode->right == nullptr) {
//                     currNode->right = new RBNode(kv, nullptr, nullptr, RED);
//                     mSize++;
//                     inserted = true;
//                 }
//                 currNode = currNode->right;
//             }

//             if (inserted) {
//                 break;
//             }
//         }

//         //
//         // at this point, we've inserted, and currNode points to the new node
//         //

//         // if (currNode->parent->colour == RED) {
//         //     // recolour if necessary
//         //     recolour(currNode);
//         // }

//         return currNode;
//     }

//     bool erase(const K& key) {
//         RBNode* node = find(key);
//         if (node == nullptr) {
//             return;
//         }

//         // two children
//         if (node->left && node->right) {

//         }

//         // one child
//         if (node->left || node->right) {

//         }
        
//         // leaf node
//         if (node->left == nullptr && node->right == nullptr) {
//             RBNode* parent = node->parent;
//             if (parent->left == node) {
//                 parent->left = nullptr;
//             } else {
//                 parent->right = nullptr;
//             }
//             delete node;
//             return;
//         }
//     }

//     RBNode* find(const K& key) {
//         if (mSize == 0) {
//             return nullptr;
//         }

//         RBNode* currNode = tree->root;
//         while (true) {
//             if (currNode == nullptr) {
//                 return nullptr;
//             }

//             if (kv.first == currNode->kv.first) {
//                 // found key
//                 return currNode;
//             }

//             if (kv.first < currNode->kv.first) {
//                 // left
//                 currNode = currNode->left;
//             } else {
//                 // right
//                 currNode = currNode->right;
//             }
//         }
//     }

//     void clear() {

//     }

// private:

//     /**
//      * Recolouring rules:
//      *      - if x parent is black: terminate
//      *      - if x parent is red:
//      *          - if x uncle red:
//      *              - easy re-colouring
//      *              - parent and uncle to black, grandfather to red
//      *              - x == grandfather, repeat steps
//      *          - if x uncle black:
//      *              - 4 cases:
//      *                  - left-left
//      *                  - left-right
//      *                  - right-right
//      *                  - right-left
//      */

//     // only works for insertion right now
//     void recolour(RBNode* node) {
//         RBNode* parent = node->parent;
//         if (parent->left == node) {
//             // `node` is left child - uncle is right child of 
//         }
//     }
// };

}; // end of 'rack'
