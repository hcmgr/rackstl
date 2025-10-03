#include <vector>
#include <queue>

namespace rack {

//
// priority_queue implemented as a max heap.
//
// Max heap is full binary tree where all children and <= parent.
//
template <class V>
class priority_queue {
private:
    std::vector<V> container;

public:
    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////
    priority_queue() = default;

    priority_queue(const priority_queue& other)
        : container(other.container) {}

    priority_queue(priority_queue&& other)
        : container(std::move(other.container)) {}

    ~priority_queue() = default;

    priority_queue& operator=(const priority_queue& other) { 
        container = other.container; 
        return *this;
    }
    priority_queue& operator=(priority_queue&& other) { 
        container = std::move(other.container); 
        return *this;
    }

    //////////////////////////////////////////////////////
    // Access
    //////////////////////////////////////////////////////
    V& top() {
        return container.front();
    }

    //////////////////////////////////////////////////////
    // Capacity
    //////////////////////////////////////////////////////
    bool empty() {
        return container.empty();
    }

    uint32_t size() {
        return container.size();
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    void push(const V& val) {
        emplace(val);
    }

    void push(V&& val) {
        emplace(val);
    }

    template <class ...Args>
    void emplace(Args&&... args) {
        int freeIdx = container.size();
        container.insert(container.begin() + freeIdx, std::forward<Args>(args)...);
        upHeap(freeIdx);
    }

    void pop() {
        if (container.empty()) {
            return;
        }
        std::swap(container.front(), container.back());
        container.pop_back();
        downHeap(0);
    }

    void swap(priority_queue& other) {
        std::swap(container, other.container);
    }

    std::string toString() {
        if (container.empty()) {
            return "[EMPTY]";
        }
        std::ostringstream oss;
        toStringHelper(0, 0, oss);
        return oss.str();
    }

    //////////////////////////////////////////////////////
    // Helpers
    //////////////////////////////////////////////////////

    bool validHeap() {
        int i = 0;
        int N = container.size();
        int l, r;
        while (i < N) {
            l = 2*i + 1;
            r = 2*i + 2;
            if (l < N && container[i] < container[l]) {
                return false;
            }
            if (r < N && container[i] < container[r]) {
                return false;
            }
            i++;
        }
        return true;
    }

    std::vector<V> getContainer() {
        return container;
    }

private: 
    void upHeap(int i) {
        if (i == 0) {
            return;
        }

        int p = parentIndex(i);
        if (container[p] < container[i]) {
            // max heap property violated
            std::swap(container[p], container[i]);
            upHeap(p);
        }
    }

    void downHeap(int i) {
        int l = 2*i + 1;
        int r = 2*i + 2;
        int N = container.size();

        int swapInd = -1;
        if (l < N && container[l] > container[i]) {
            swapInd = l;
        }
        if (r < N && container[r] > container[i]) {
            if (swapInd != -1 && container[r] > container[l]) {
                swapInd = r;
            }
        }
        if (swapInd == -1) {
            return;
        }

        std::swap(container[swapInd], container[i]);
        downHeap(swapInd);
    }

    int parentIndex(int i) {
        return (i - 1) / 2;
    }

    void toStringHelper(int idx, int depth, std::ostringstream& oss) const {
        if (idx >= container.size()) return;

        int right = 2 * idx + 2;
        int left  = 2 * idx + 1;

        toStringHelper(right, depth + 1, oss);

        // indent by depth
        for (int i = 0; i < depth; i++) {
            oss << "    ";
        }
        oss << container[idx] << "\n";

        toStringHelper(left, depth + 1, oss);
    }
};
}; // end of 'rack'