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
    
    RBNode(
        const std::pair<K, V>& kvArg,
        bool colour)
        : kv(kvArg), parent(nullptr), left(nullptr), right(nullptr), colour(colour) {}
};

template <class K, class V>
class RBTree {
public:
    using Node = RBNode<K, V>;

    Node* root;
    uint32_t mSize;

    RBTree() 
        : root(nullptr), mSize(0) {}
    
    ~RBTree() {
        clear();
    }
    
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

        assert(inserted);

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
            Node* succ = RBTree::inorderSuccessor(node);
            std::swap(node->kv, succ->kv);
            node = succ;
        }

        if (node->left) {
            // has left child - directly connect parent and left child
            updateParentRef(node, node->left);
            delete node;
            mSize--;
            return true;
        }

        if (node->right) {
            // has right child - directly connect parent and right child
            updateParentRef(node, node->right);
            delete node;
            mSize--;
            return true;
        }
        
        if (node->left == nullptr && node->right == nullptr) {
            // is leaf node - just remove it
            updateParentRef(node, nullptr);
            delete node;
            return true;
        }

        assert(false); // never get here
    }

    void fixDeletion(Node* x, Node* parent) {
        // todo
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

    //
    // Returns in-order successor of `target`.
    // If `target` has no in-order successor, nullptr is returned (i.e. if 
    // `target` is largest node, or `target` is nullptr).
    //
    static Node* inorderSuccessor(Node* target) {
        if (target == nullptr) return nullptr;

        //
        // Case 1: right subtree exists.
        // In-order successor is least (furthest left) node in right-subtree.
        //
        if (target->right != nullptr) {
            Node* curr = target->right;
            while (curr->left != nullptr) {
                curr = curr->left;
            }
            return curr;
        }

        //
        // Case 2: no right subtree.
        // In-order successor is lowest ancestor that is a left child.
        //
        Node* curr = target;
        Node* p = curr->parent;
        while (p != nullptr && curr == p->right) {
            curr = p;
            p = p->parent;
        }

        return p; // either parent of left child, or nullptr
    }

    //
    // Returns in-order predecessor of `node`
    //
    static Node* inorderPredecessor(Node* target) {
        if (target == nullptr) return nullptr;

        //
        // Case 1: left subtree exists.
        // In-order predecessor is largest (further right) in left subtree
        //
        if (target->left != nullptr) {
            Node* curr = target->left;
            while (curr->right) {
                curr = curr->right;
            }
            return curr;
        }

        //
        // Case 2: no left subtree.
        // In-order precedessor is lowest ancestor that is a right child
        //
        Node* curr = target;
        Node* p = curr->parent;
        while (p != nullptr && p->left == curr) {
            curr = p;
            p = curr->parent;
        }

        return p; // either parent of left child, or nullptr (i.e. no inorder predecessor exists)
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

    void rotateLeft(Node* root) {
        if (!root || !root->right) return;

        Node* right = root->right;
        root->right = right->left;
        if (right->left) {
            right->left->parent = root;
        }

        right->parent = root->parent;
        updateParentRef(root, right);

        right->left = root;
        root->parent = right;

        if (this->root == root) {
            this->root = right;
        }
    }

    void rotateRight(Node* root) {
        if (!root || !root->left) return;

        Node* left = root->left;
        root->left = left->right;
        if (left->right) {
            left->right->parent = root;
        }

        left->parent = root->parent;
        updateParentRef(root, left);

        left->right = root;
        root->parent = left;

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

        std::runtime_error("getUncle - unreachable path reached"); // never reach here
        return nullptr;
    }

    bool checkRbTree() {
        if (this->root == nullptr) {
            MyLog("null root");
            return true;
        }
        if (this->root->colour != BLACK) {
            MyLog("invalid rb tree - non-black root");
            return false;
        }
        return checkRbTreeHelper(this->root).first;
    }

    std::pair<bool, int> checkRbTreeHelper(Node* node) {
        // nil node
        if (node == nullptr) {
            return {true, -1};
        }

        // check no double red
        if (node->colour == RED) {
            bool doubleRed = (node->left && node->left->colour == RED) ||
                             (node->right && node->right->colour == RED);
            if (doubleRed) {
                // double red - finish early
                MyLog("invalid rb tree - double red");
                return {false, -1};
            }
        }

        // check equal black height
        auto l = checkRbTreeHelper(node->left);
        auto r = checkRbTreeHelper(node->right);
        if (!l.first || !r.first) {
            // subtree failed - finish early
            return {false, -1};
        }

        bool equalBlackHeight = l.second == r.second;
        if (!equalBlackHeight) {
            // un-equal black height - finish early
            MyLog("invalid rb tree - un-equal black height");
            return {false, -1};
        }

        int blackHeight = l.second;
        if (node->colour == BLACK) {
            blackHeight += 1;
        }

        return {true, blackHeight};
    }

private:
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

        if (newNode != nullptr) {
            newNode->parent = parent;
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
        oss << std::string(depth * 8, ' ')   // 4 spaces per depth level
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
    using Node = RBNode<K, V>;

public:
    class iterator; // forward-declare iterator

    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////
    map() {
        tree = new RBTree<K, V>();
    }

    ~map() {
        // tree free'd automatically
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
        return tree->size() == 0;
    }

    uint32_t size() {
        return tree->size();
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////
    void clear() {
        tree->clear();
    }


    void insert(std::pair<K, V> kv) {

    }

    template <typename... Args>
    void emplace(Args&&... args);

    iterator erase(iterator pos) {
        
    }

    //////////////////////////////////////////////////////
    // Lookup
    //////////////////////////////////////////////////////
    uint32_t count(const K& key) const {
    }

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
    public:
        Node* node;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = std::pair<const K, V>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        // constructors
        iterator() : node(nullptr) {}
        iterator(Node* n) : node(n) {}

        // Dereference
        reference operator*() const { return node->kv; }
        pointer operator->() const { return &(node->kv); }

        iterator& operator++() { // pre-inc
            Node* succ = RBTree<K, V>::inorderSuccessor(node);
            return iterator(succ);
        }

        iterator operator++(int) { // post-inc
            iterator tmp = this;
            --this;
            return tmp;
        }

        iterator& operator--() { // pre-dec
            Node* pre = RBTree<K, V>::inorderPredecessor(node);
            return iterator(succ);
        }
        iterator  operator--(int) { // post-dec
            iterator tmp = this;
            --this;
            return tmp;
        }
    };

    iterator begin() {
        return {}
    }
    iterator end() {

    }

    //////////////////////////////////////////////////////
    // Helpers
    //////////////////////////////////////////////////////
};

}; // end of 'rack'
