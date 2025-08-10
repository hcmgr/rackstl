#include <memory>

#define PAGE_SIZE 4096

namespace rack {

template <class T>
class deque {
private:

    //
    // The underlying data structure is an array of fixed-size 'chunks'.
    // Together, the chunks are treated as one large buffer. We keep pointers
    // to the front and back for O(1) access. As deque grows and shrinks, chunks
    // are added and removed as needed. So, compared to vector, insertion/deletion 
    // is still O(1) on average, but resizing is faster. For instance, increasing 
    // deque's capacity is done by:
    //      - allocating a new chunk AND;
    //      - copying the chunk pointers into a new container.
    // Thus, resize'ing is O(nChunks), rather than vector's O(n).
    //
    // Of course, the price you pay is slower randmom access, as the 
    // chunks are not contiguous in memory.
    //
    T** chunkMap;
    uint32_t nChunks;
    uint32_t chunkSize;
    uint32_t _size;

    // Front and back pointers -  chunk index + offset in chunk
    uint32_t frontChunk, frontOff; 
    uint32_t backChunk, backOff;

    std::allocator<T*> chunkAllocator;
    std::allocator<T> elementAllocator;

    friend class DequeTests; // for debugging purposes

public:

    //////////////////////////////////////////////////////
    // Construtors
    //////////////////////////////////////////////////////

    deque(uint32_t chunkSizeBytes = PAGE_SIZE) {
        nChunks = 1;
        chunkSize = chunkSizeBytes / sizeof(T);
        _size = 0;

        chunkMap = chunkAllocator.allocate(nChunks);
        chunkMap[0] = elementAllocator.allocate(chunkSize);

        frontChunk = 0;
        frontOff = chunkSize / 2;

        backChunk = 0;
        backOff = chunkSize / 2;
    }

    ~deque() {

    }

    //////////////////////////////////////////////////////
    // Accessors
    //////////////////////////////////////////////////////

    T& front() {
        return chunkMap[frontChunk][frontOff];
    }

    T& back() {
        return chunkMap[backChunk][backOff];
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    void push_front(const T& val) {
        // front is at limit => resize needed
        if (frontChunk == 0 && frontOff == 0) {
            grow();
        }

        //
        // move front pointer to new position
        //

        if (_size > 0) { // don't move pointer if first element being added
            if (frontOff == 0) {
                frontChunk -= 1;
                frontOff = chunkSize - 1;

                // lazily allocate new chunk (if needed)
                if (chunkMap[frontChunk] == nullptr) {
                    chunkMap[frontChunk] = elementAllocator.allocate(chunkSize);
                }
            } else {
                frontOff -= 1;
            }
        }

        // push copy of val
        elementAllocator.construct(chunkMap[frontChunk] + frontOff, val);
        _size++;
    }

    void push_back(const T& val) {
        // back is at limit => resize needed
        if (backChunk == nChunks - 1 && backOff == chunkSize - 1) {
            grow();
        }

        //
        // move back pointer to new position
        //

        if (_size > 0) { // don't move pointer if first element being added
            if (backOff == chunkSize - 1) {
                backChunk += 1;
                backOff = 0;

                // lazily allocate new chunk (if needed)
                if (chunkMap[backChunk] == nullptr) {
                    chunkMap[backChunk] = elementAllocator.allocate(chunkSize);
                }
            } else {
                backOff += 1;
            }
        }

        // push copy of val
        elementAllocator.construct(chunkMap[backChunk] + backOff, val);
        _size++;
    }

    void pop_front() {
        // de-allocate object
        elementAllocator.destroy(chunkMap[frontChunk] + frontOff);
        _size -= 1;

        // removed last element - don't move the front pointer
        if (_size == 0) {
            return;
        }

        // already at back - don't move the front pointer
        if (frontChunk == nChunks - 1 && frontOff == chunkSize - 1) {
            return;
        }

        // move the front pointer
        if (frontOff == chunkSize - 1) {
            frontChunk -= 1;
            frontOff = 0;
        } else {
            frontOff += 1;
        }
    }

    void pop_back() {
        // de-allocate object
        elementAllocator.destroy(chunkMap[backChunk] + backOff);
        _size -= 1;

        // removed last element - don't move the back pointer
        if (_size == 0) {
            return;
        }

        // already at front - don't move the back pointer
        if (backChunk == 0 && backOff == 0) {
            return;
        }

        // move the back pointer
        if (backOff == 0) {
            backChunk -= 1;
            backOff = chunkSize - 1;
        } else {
            backOff -= 1;
        }
    }

    void resize();

    void clear();

    //////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////

    std::string to_string() 
    {
        std::ostringstream oss;
        oss << "-------------------------------" << "\n";
        oss << "Num chunks: " << nChunks << "\n";
        oss << "Chunk size: " << chunkSize << "\n";
        oss << "Front: " << frontChunk << " " << frontOff << "\n";
        oss << "Back: " << backChunk << " " << backOff << "\n";
        for (int i = 0; i < nChunks; i++) {
            if (chunkMap[i] == nullptr) {
                oss << "[]";
                if (i < nChunks - 1) {
                    oss << ", ";
                }
                continue;
            }

            oss << "[";
            for (int j = 0; j < chunkSize; j++) {
                oss << chunkMap[i][j];
                if (j < chunkSize - 1) {
                    oss << ",";
                }
            }
            oss << "]";
            if (i < nChunks - 1) {
                oss << ", ";
            }
        }
        oss << "\n";
        oss << "-------------------------------" << "\n";
        return oss.str();
    }

    //////////////////////////////////////////////////////
    // Capacity
    //////////////////////////////////////////////////////

    bool empty() { return _size == 0; }
    uint32_t size() { return _size; }

private:
    // Grow the chunk map by 2x. Re-centre the existing pointers.
    void grow() {
        // allocate new 2x map
        uint32_t newnChunks = nChunks * 2;
        T** newChunkMap = chunkAllocator.allocate(newnChunks);

        // copy chunk pointers to center of the map
        uint32_t centerOff = newnChunks / 4;
        for (int i = 0; i < newnChunks; i++) {
            if (i >= centerOff && i < newnChunks - centerOff) {
                newChunkMap[i] = chunkMap[i - centerOff];
            } else {
                newChunkMap[i] = nullptr;
            }
        }

        // de-allocate old map and replace with new one
        chunkAllocator.deallocate(chunkMap, nChunks);
        chunkMap = newChunkMap;
        nChunks = newnChunks;

        // update front and back pointers after resize
        frontChunk += centerOff;
        backChunk += centerOff;
    }
};

};