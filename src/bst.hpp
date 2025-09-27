#pragma once
#include <memory>
#include <algorithm>
#include <ostream>
#include <cassert>
#include "vector.hpp"

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
            Node* succ = BSTTree::inorderSuccessor(node);
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

        return p; // either parent of left child, or nullptr
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
    }

    void rotateLeftRight(Node* parent, Node* grandParent) {
        rotateLeft(parent);
        rotateRight(grandParent);
    }

    void rotateRightRight(Node* grandParent) {
        rotateLeft(grandParent);
    }

    void rotateRightLeft(Node* parent, Node* grandParent) {
        rotateRight(parent);
        rotateLeft(grandParent);
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

        // current node (indented according to `depth`)
        oss << std::string(depth * 4, ' ')   // 4 spaces per depth level
            << "(" << node->kv.first << ":" << node->kv.second << ")\n";

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
};