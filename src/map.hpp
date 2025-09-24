#pragma once
#include <memory>
#include <algorithm>

#include "vector.hpp"

#define BLACK   0
#define RED     1

namespace rack{

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

template <class K, class V>
struct RBNode {
    std::pair<K, V> kv;
    RBNode* parent;
    RBNode* left;
    RBNode* right;
    bool colour;

    RBNode(
        const std::pair<K, V>& kvArg, 
        RBNode* parentArg, 
        RBNode* leftArg, 
        RBNode* rightArg,
        bool colour)
        : kv(kvArg), parent(parentArg), left(leftArg), right(rightArg), colour(colour) {}
};

template <class K, class V>
class RBTree {
public:
    using Node = RBNode<K, V>;

    Node* root;
    uint32_t mSize;

    RBTree() 
        : root(nullptr), mSize(0) {}
    
    // 
    // Returns pointer to newly inserted node, or existing node if it already exists.
    //
    Node* insert(std::pair<K, V> kv) {
        if (mSize == 0) {
            root = new Node(kv, nullptr, nullptr, nullptr, BLACK);
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
                    currNode->left = new Node(kv, currNode, nullptr, nullptr, RED);
                    mSize++;
                    inserted = true;
                }
                currNode = currNode->left;

            } else {
                // right
                if (currNode->right == nullptr) {
                    currNode->right = new Node(kv, currNode, nullptr, nullptr, RED);
                    mSize++;
                    inserted = true;
                }
                currNode = currNode->right;
            }

            if (inserted) {
                break;
            }
        }

        //
        // At this point, we've inserted, and currNode points to the newly inserted node.
        // Now, if insertion caused red-black violations, fix.
        //
        fixInsertion(currNode);

        return currNode;
    }

    //
    // Fixes insertion via a recolour; necessary if parent is red.
    //
    // Two cases:
    //      - red uncle
    //          - parent and uncle to black
    //          - grandfather to red
    //          - continue steps with grandfather
    //      - black uncle
    //          - 4 cases:
    //              - left-left
    //              - left-right
    //              - right-right
    //              - right-left
    //
    // Note that red parent => guaranteed to have non-null grandparent.
    // So uncle either exists, or null (treated as black).
    //
    void fixInsertion(Node* currNode) {
        // while parent exists and is red => violation
        while (currNode != root && currNode->parent->colour == RED) {
            Node* parent = currNode->parent;
            Node* grandParent = parent->parent;
            Node* uncle = getUncle(currNode);

            // 
            // red uncle
            //
            if (uncle && uncle->colour == RED) {
                parent->colour = BLACK;
                uncle->colour = BLACK;
                grandParent->colour = RED;

                // move violation up the tree, keep going
                currNode = grandParent;
                continue;
            }

            //
            // black uncle - 4 cases
            //
            if (!uncle || uncle->colour == BLACK) {
                if (isLeftChild(parent) && isLeftChild(currNode)) {
                    rotateLeftLeft(parent->parent);
                } 
                else if (isLeftChild(parent) && isRightChild(currNode)) {
                    rotateLeftRight(parent, parent->parent);
                }
                else if (isRightChild(parent) && isRightChild(currNode)) {
                    rotateRightRight(parent->parent);
                }
                else if (isRightChild(parent) && isLeftChild(currNode)) {
                    rotateRightLeft(parent, parent->parent);
                }
                else {
                    // never reach here
                    throw std::runtime_error("Black uncle - incorrect parent-grandparent relationship");
                }

                break; // black uncle terminates here
            }
        }

        // always enforce black root
        root->colour = BLACK;
    }

    Node* getUncle(Node* node) {
        Node* parent = node->parent;
        if (parent == nullptr) return nullptr;
        Node* grandParent = parent->parent;
        if (grandParent == nullptr) return nullptr;

        if (grandParent->left == parent) {
            return grandParent->right;
        } else if (grandParent->right == parent) {
            return grandParent->left;
        } 

        std::runtime_error("getUncle - unreachable path reached"); // never go here
        return nullptr;
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

    // Returns side-ways view of BST
    std::string toString() {
        std::ostringstream oss;
        if (root == nullptr) {
            oss << "[EMPTY]" << "\n";
            return oss.str();
        } else {
            toStringHelper(root, oss, 0);
        }
        return oss.str();
    }

    //////////////////////////////////////////////////////
    // Rotations
    //////////////////////////////////////////////////////

    // Rotates tree rooted at `root` left
    void rotateLeft(Node* root) {
        if (root == nullptr) return;

        Node* right = root->right;
        if (right == nullptr) return;

        // root's right becomes right's left
        root->right = right->left;

        // right's left becomes root
        right->left = root;

        updateParentRef(root, right);

        // update parents
        right->parent = root->parent;
        root->parent = right;

        // update global root, if necessary
        if (this->root == root) {
            this->root = right;
        }
    }

    // Rotates tree rooted at `root` right
    void rotateRight(Node* root) {
        if (root == nullptr) return;

        Node* left = root->left;
        if (left == nullptr) return;

        // root's left becomes left's right
        root->left = left->right;

        // left's right becomes root
        left->right = root;

        updateParentRef(root, left);

        // update parents
        left->parent = root->parent;
        root->parent = left;

        // update global root, if necessary
        if (this->root == root) {
            this->root = left;
        }
    }

    void rotateLeftLeft(Node* grandParent) {
        rotateRight(grandParent);
        std::swap(grandParent->colour, grandParent->parent->colour);
    }

    void rotateLeftRight(Node* parent, Node* grandParent) {
        rotateLeft(parent);
        rotateLeftLeft(grandParent);
    }

    void rotateRightRight(Node* grandParent) {
        rotateLeft(grandParent);
        std::swap(grandParent->colour, grandParent->parent->colour);
    }

    void rotateRightLeft(Node* parent, Node* grandParent) {
        rotateRight(parent);
        rotateRightRight(grandParent);
    }

private:
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

    //
    // Updates parent of `oldNode` to point to `newNode` instead
    //
    void updateParentRef(Node* oldNode, Node* newNode) {
        Node* parent = oldNode->parent;
        if (parent == nullptr) {
            return;
        }

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
        std::string colourCode = node->colour == RED ? "R" : "B";

        // current node (indented according to `depth`)
        oss << std::string(depth * 4, ' ')   // 4 spaces per depth level
            << "(" << node->kv.first << ":" << colourCode << ")\n";

        // left subtree
        toStringHelper(node->left, oss, depth + 1);
    }

    bool isLeftChild(Node* node) {
        if (node == nullptr) return false;
        Node* parent = node->parent;
        if (parent == nullptr) return false;
        return parent->left == node;
    }

    bool isRightChild(Node* node) {
        if (node == nullptr) return false;
        Node* parent = node->parent;
        if (parent == nullptr) return false;
        return parent->right == node;
    }
};

template <class K, class V, class Alloc = std::allocator<std::pair<const K, V>>>
class map {
private:
    RBTree<K, V>* tree;

public:
    class iterator; // forward-declare iterator

    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////
    map() {
        tree = new RBTree<K, V>();
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

    iterator find(const K& key) {}

    bool contains(const K& key) {}

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

}; // end of 'rack'
