#include <memory>

#define PAGE_SIZE 4096

namespace rack {

template <class T>
class deque {
private:

    //
    // The underlying data structure is an array of fixed-sized 'chunks'.
    // Together, the chunks are treated like a circular buffer. As deque grows
    // and shrinks, chunks are added and removed as needed. So, compared to vector,
    // insertion/deletion is still O(1) on average, but resizing is faster.
    // For instance, increasing deque's capacity is done by:
    //      - allocating a new chunk AND;
    //      - copying the chunk pointers into a new container.
    // Thus, resize'ing is O(nChunks), rather than vector's O(n).
    //
    // Of course, the price you pay is slower randmom access, as the 
    // chunks are not contiguous with respect to each other.
    //
    T** chunkList;
    uint32_t nChunks;
    uint32_t chunkSize;
    uint32_t _capacity; // nChunks * chunkSize
    uint32_t _size;

    //
    // Indices are across the whole structure. For example, if:
    //
    // nChunks == 4, chunkSize == 5, frontIdx == 11, then:
    // 
    // front chunk ind == frontIdx / chunkSize == 2
    // front chunk position == frontIdx % chunkSize == 1
    //
    uint32_t frontIdx;
    uint32_t backIdx;
    
    std::allocator<T*> chunkAllocator;
    std::allocator<T> elementAllocator;

public:

    //////////////////////////////////////////////////////
    // Construtors
    //////////////////////////////////////////////////////

    deque() {
        // Initialise using one chunk. Chunks are one page in size, assuming 4KB pages.
        nChunks = 1;
        chunkSize = PAGE_SIZE / sizeof(T);
        _capacity = nChunks * chunkSize;

        allocateChunkList();

        frontIdx = 0;
        backIdx = 0;
    }

    ~deque() {

    }

    //////////////////////////////////////////////////////
    // Accessors
    //////////////////////////////////////////////////////

    T& front() {
        assert(frontIdx < _capacity);
        uint32_t chunk = frontIdx / chunkSize;
        uint32_t pos = frontIdx % chunkSize;
        return chunkList[chunk][pos];
    }

    T& back() {
        assert(frontIdx < _capacity);
        uint32_t chunk = backIdx / chunkSize;
        uint32_t pos = backIdx % chunkSize;
        return chunkList[chunk][pos];
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    //
    // Adds copy of `val` to back of the container. If a new chunk is needed,
    // one is allocateed, and the chunk pointer list grows via a doubling strategy.
    //
    void push_back(const T& val) {
    }

    void push_front(const T& val);

    void pop_back();

    void pop_front();

    void resize();

    // void erase(iterator pos);
    void clear();

    //////////////////////////////////////////////////////
    // Capacity
    //////////////////////////////////////////////////////

    bool empty() { return _size == 0; }
    uint32_t size() {return _size; }

private:
    void allocateChunkList() {
        chunkList = chunkAllocator.allocate(nChunks);
        for (int i = 0; i < nChunks; i++) {
            chunkList[i] = elementAllocator.allocate(chunkSize);
        }
    }
};

};