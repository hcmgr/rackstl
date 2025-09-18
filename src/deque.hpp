#pragma once
#include <memory>
#include "utils.hpp"

#define PAGE_SIZE 4096

namespace rack {

//
// Deque uses an array of fixed-size arrays (chunks) as its underlying data structure.
// We call this the 'chunk map'.
//
// Together, the chunks are treated as one large buffer of contiguous elements. We start
// with one chunk, and as deque grows, chunks are lazily allocated when needed. The front
// grows 'left', meaning: when the front runs out of capacity, we allocate a new chunk at the
// next lowest index in the chunk map. Similarly, the back grows 'right', with new chunks allocated
// at the next highest index in the chunk map.
// 
// This lazy chunk allocation makes resizing faster than vector's. For example, increasing deque's
// chunk map capacity is done by:
//      - allocating a new chunk map AND;
//      - copying the chunk pointers into the new map
// Thus, resize'ing is O(nChunks), rather than vector's O(n).
// 
// Of course, the price you pay is slower random access, as the chunks are not contiguous
// in memory (i.e. many pointer indirections needed, rather than just a quick scan).
//
template <class T>
class deque {
private:

    T** chunkMap;

    uint32_t nChunks;
    uint32_t chunkSize;
    uint32_t _size;

    // Front and back pointers -  chunk index + offset in chunk
    uint32_t frontChunk, frontOff; 
    uint32_t backChunk, backOff;

    std::allocator<T*> chunkAllocator;
    std::allocator<T> elementAllocator;

public:
    class iterator; // forward-declare iterator

    //////////////////////////////////////////////////////
    // Construtors
    //////////////////////////////////////////////////////

    deque(uint32_t chunkSizeBytes = PAGE_SIZE) {
        nChunks = 1;
        chunkSize = chunkSizeBytes / sizeof(T);
        _size = 0;

        chunkMap = chunkAllocator.allocate(nChunks);
        chunkMap[0] = elementAllocator.allocate(chunkSize);

        // front and back pointers - chunk index and offset in chunk
        frontChunk = 0;
        frontOff = chunkSize / 2;
        backChunk = 0;
        backOff = chunkSize / 2;
    }

    // copy constructor - deep copy needed
    deque(const deque<T>& other) {

    }

    ~deque() {
        // free objects within each chunk

        // free each chunk 

        // free chunk map

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

    T& operator[](uint32_t i) {
        if (i >= _size) {
            throw std::runtime_error(
                "Index out of bounds error: " +
                std::string("index=") + std::to_string(i) + ", size=" + std::to_string(_size)
            );
        }

        int jumpChunks = i / chunkSize;
        int jumpOff = i % chunkSize;
        if (frontOff + jumpOff >= chunkSize) {
            jumpChunks++;
        }

        int chunk = frontChunk + jumpChunks;
        int off = (frontOff + jumpOff) % chunkSize;
        return chunkMap[chunk][off];
    }

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    void push_front(const T& val) {
        // front is at limit - resize needed
        if (frontChunk == 0 && frontOff == 0) {
            grow();
        }

        // update front pointer - if first el added, don't update
        if (_size > 0) {
            dec(frontChunk, frontOff);
        }

        // push copy of val
        utils::allocConstruct(elementAllocator, chunkMap[frontChunk] + frontOff, val);
        _size++;
    }

    void push_back(const T& val) {
        // back is at limit => resize needed
        if (backChunk == nChunks - 1 && backOff == chunkSize - 1) {
            grow();
        }

        // update back pointer - if first el added, don't update
        if (_size > 0) {
            inc(backChunk, backOff);
        }

        // push copy of val
        utils::allocConstruct(elementAllocator, chunkMap[backChunk] + backOff, val);
        _size++;
    }

    void pop_front() {
        // de-allocate object
        utils::allocDestroy(elementAllocator, chunkMap[frontChunk] + frontOff);
        _size -= 1;

        // removed last element - don't move the front pointer
        if (_size == 0) {
            return;
        }

        // already at back - don't move the front pointer
        if (frontChunk == nChunks - 1 && frontOff == chunkSize - 1) {
            return;
        }

        // update the front pointer
        inc(frontChunk, frontOff);
    }

    void pop_back() {
        // de-allocate object
        utils::allocDestroy(elementAllocator, chunkMap[backChunk] + backOff);
        _size -= 1;

        // removed last element - don't move the back pointer
        if (_size == 0) {
            return;
        }

        // already at front - don't move the back pointer
        if (backChunk == 0 && backOff == 0) {
            return;
        }

        // update the back pointer
        dec(backChunk, backOff);
    }

    // Insert copy of `val` at position before `loc`.
    void insert(iterator loc, const T& val) {
        if (loc < begin() || loc >= end()) { 
            std::cout << "loc out of range" << "\n";
            return; 
        }

        // add at front
        if (loc == begin()) {
            return push_front(val);
        }

        // add at end
        if (loc == end()) {
            return push_back(val);
        }

        //
        // otherwise, we:
        //      - split container at `loc`
        //      - shift the shorter side (left or right) out one position
        //      - insert `val` in vacant slot
        //

        uint32_t locChunk = loc.chunk - chunkMap;
        uint32_t locOff = loc.pos - *loc.chunk;

        // pick shorter side (left or right)
        int distToFront = (locChunk - frontChunk) * chunkSize + (locOff - frontOff);
        int distToBack = (backChunk - locChunk) * chunkSize + (backOff - locOff);
        bool left = distToFront < distToBack; // 1 - left, 0 - right

        if (left) {
            // no room on left side - grow before we shift
            if (frontChunk == 0 && frontOff == 0) {
                grow(); 

                // re-calculate `loc` in new map
                loc = begin() + distToFront;
                locChunk = loc.chunk - chunkMap;
                locOff = loc.pos - *loc.chunk;
            }

            // shift elements in range [front, loc) left one position (excluding loc)
            dec(locChunk, locOff);
            shiftLeft(frontChunk, frontOff, locChunk, locOff);
            dec(frontChunk, frontOff);

        } else {
            // no room on right-side - grow before we shift
            if (backChunk == nChunks - 1 && backOff == chunkSize - 1) {
                grow();

                // re-calculate `loc` in new map
                loc = begin() + distToFront;
                locChunk = loc.chunk - chunkMap;
                locOff = loc.pos - *loc.chunk;
            }

            // shift elements in range [loc, back] right one position (including loc)
            shiftRight(locChunk, locOff, backChunk, backOff);
            inc(backChunk, backOff);
        }

        // insert `val`
        chunkMap[locChunk][locOff] = val;
        _size++;
    }

    // Erase element at `loc`.
    void erase(iterator loc) {
        if (loc < begin() || loc >= end()) { 
            std::cout << "loc out of range" << "\n";
            return; 
        }

        // remove front element
        if (loc == begin()) {
            utils::allocDestroy(elementAllocator, chunkMap[frontChunk] + frontOff);
            chunkMap[frontChunk][frontOff] = T();
            inc(frontChunk, frontOff);
            _size--;
            return;
        }

        // remove back element
        if (loc == end() - 1) {
            utils::allocDestroy(elementAllocator, chunkMap[backChunk] + backOff);
            chunkMap[backChunk][backOff] = T();
            dec(backChunk, backOff);
            _size--;
            return;
        }

        //
        // otherwise, we:
        //      - split container at `loc`
        //      - shift the shorter side (left or right) in one position
        //      - insert `val` in vacant slot
        //

        uint32_t locChunk = loc.chunk - chunkMap;
        uint32_t locOff = loc.pos - *loc.chunk;

        // pick shorter side (left or right)
        int distToFront = (locChunk - frontChunk) * chunkSize + (locOff - frontOff);
        int distToBack = (backChunk - locChunk) * chunkSize + (backOff - locOff);
        bool left = distToFront < distToBack; // 1 - left, 0 - right

        // de-allocate object
        utils::allocDestroy(elementAllocator, chunkMap[locChunk] + locOff);

        if (left) {
            // shift elements in range [front, loc) in one position
            dec(locChunk, locOff);
            shiftRight(frontChunk, frontOff, locChunk, locOff);
            inc(frontChunk, frontOff);
        } else {
            // shift elements in range (loc, loc] in one position
            inc(locChunk, locOff);
            shiftLeft(locChunk, locOff, backChunk, backOff);
            dec(backChunk, backOff);
        }

        _size--;
    }

    //
    // Resize container to `count` elements.
    //
    // If `count` is equal current size, do nothing.
    // If `count` is greater than current size, additional copies of `val` are appended.
    // If `count` less than current size, container reduced to first `count` elements.
    //
    void resize(uint32_t count, const T& val) {
        if (count == _size) {
            // do nothing
            return;
        } else if (count > _size) {
            // append additional copies of T()
            while (count > _size++) {
                push_back(val);
            }
        } else {
            // reduce to first `count` elements
            while (count < _size--) {
                pop_back();
            }
        }
    }

    void resize(uint32_t count) {
        return resize(count, T());
    }

    //
    // Clears the container (i.e. size() == 0).
    //
    // Container capacity is preserved (i.e. chunks are not free'd). To
    // free the underlying memory, call shrink_to_fit() after clear().
    //
    void clear() {
        while (_size > 0) {
            erase(begin());
        }
    }

    //////////////////////////////////////////////////////
    // Iterators
    //////////////////////////////////////////////////////

    class iterator {
    public:
        T* pos;             // position in chunk
        T** chunk;          // chunk pointer
        int32_t chunkSize; // chunk size - TODO: replace this with compile-time variable

        // typedefs - necessary for other STL functions to use this (e.g. std::sort)
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        // constructor
        iterator(T* pos, T** chunk, int32_t chunkSize) 
            : pos(pos), chunk(chunk), chunkSize(chunkSize) {}
        
        // copy constructor
        iterator(const iterator& other)
            : pos(other.pos), chunk(other.chunk), chunkSize(other.chunkSize) {}

        // dereference
        T& operator*() const { return *pos; }
        T* operator->() const { return pos; }

        //
        // comparison
        //

        bool operator==(const iterator& other) const { 
            return chunk == other.chunk && pos == other.pos; 
        }
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

        bool operator<(const iterator& other) const { 
            return chunk < other.chunk || (chunk == other.chunk && pos < other.pos);
        }

        bool operator<=(const iterator& other) const { 
            return *this < other || *this == other;
        }

        bool operator>(const iterator& other) const { 
            return other < *this;
        }

        bool operator>=(const iterator& other) const { 
            return !(*this < other);
        }

        //
        // arithmetic
        //

        iterator operator+(int32_t i) const { 
            iterator newIt = *this;
            newIt += i;
            return newIt;
        }

        iterator operator-(int32_t i) const { 
            iterator newIt = *this;
            newIt -= i;
            return newIt;
        }

        iterator& operator+=(int32_t i) {
            if (i == 0) return *this;
            
            int32_t currentPos = pos - *chunk;
            int32_t newPos = currentPos + i;
            
            if (newPos >= 0 && newPos < chunkSize) {
                // move within chunk
                pos = *chunk + newPos;
            } else {
                // cross chunk boundary
                int32_t chunkOffset = 0;
                if (newPos >= 0) {
                    // positive case
                    chunkOffset = newPos / chunkSize;
                    newPos = newPos % chunkSize;
                } else {
                    // negative case
                    chunkOffset = (newPos + 1) / chunkSize - 1;
                    newPos = newPos - chunkOffset * chunkSize;
                }
                
                chunk += chunkOffset;
                pos = *chunk + newPos;
            }
            
            return *this;
        }

        iterator& operator-=(int32_t i) {
            return *this += (-i);
        }

        int32_t operator-(const iterator& other) const {
            int32_t chunkDiff = chunk - other.chunk;
            int32_t posDiff = (pos - *chunk) - (other.pos - *(other.chunk));
            return chunkDiff * chunkSize + posDiff;
        }

        // pre-increment
        iterator& operator++() { 
            if (pos - *chunk == chunkSize - 1) {
                chunk++;
                pos = *chunk;
            } else {
                pos++;
            }
            return *this;
        } 

        // post-increment
        iterator operator++(int) { 
            iterator tmp = *this; // copy old iterator
            ++(*this);
            return tmp;
        }

        // pre-decrement
        iterator& operator--() {
            if (pos == *chunk) {
                chunk--;
                pos = *chunk + chunkSize - 1;
            } else {
                pos--;
            }
            return *this;
        }

        // post-decrement
        iterator operator--(int) {
            iterator tmp = *this; // copy old iterator
            --(*this);
            return tmp;
        }

        // index
        T& operator[](int32_t i) const {
            return *(*this + i);
        }

        std::string to_string() const {
            std::ostringstream oss;
            oss << "chunk front: " << *chunk << "\n";
            oss << "pos: " << pos << "\n";
            oss << "chunk: " << chunk << "\n";
            oss << "\n";
            return oss.str();
        }
    };

    // iterator pointing to front
    iterator begin() {
        return iterator(
            *(chunkMap + frontChunk) + frontOff, 
            chunkMap + frontChunk,
            chunkSize
        );
    }

    // iterator pointing to one past the back
    iterator end() {
        if (backOff == chunkSize - 1) {
            return iterator(
                *(chunkMap + backChunk + 1),
                chunkMap + backChunk + 1,
                chunkSize
            );
        }
        return iterator(
            *(chunkMap + backChunk) + backOff + 1, 
            chunkMap + backChunk,
            chunkSize
        );
    }

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
        oss << "Size: " << _size << "\n";
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

    //
    // Requests removal of un-used capacity (i.e. to reduce memory usage).
    // 
    // For this implementation, that means de-allocating un-used chunks.
    // Our implementation ensures elements are always logically contiguous, 
    // and centred in the container. Therefore, the un-used chunks will always
    // be at the ends.
    //
    void shrink_to_fit() {
        // de-allocate un-used chunks before front
        int32_t currChunk = frontChunk - 1;
        while (currChunk >= 0) {
            elementAllocator.deallocate(chunkMap[currChunk], chunkSize);
            currChunk--;
        }

        // de-allocate un-used chunks after back
        currChunk = backChunk + 1;
        while (currChunk < nChunks) {
            elementAllocator.deallocate(chunkMap[currChunk], chunkSize);
            currChunk++;
        }

        // allocate new chunkMap for number of used chunks
        uint32_t nUsedChunks = backChunk - frontChunk + 1;
        T** newChunkMap = chunkAllocator.allocate(nUsedChunks);

        // copy old chunk pointers into new map
        currChunk = frontChunk;
        for (int i = 0; i < nUsedChunks; i++) {
            newChunkMap[i] = chunkMap[currChunk];
            currChunk++;
        }

        // de-allocate old map and replace with new one
        chunkAllocator.deallocate(chunkMap, nChunks);
        chunkMap = newChunkMap;
        nChunks = nUsedChunks;

        // update front and back pointers after resize
        frontChunk = 0;
        backChunk = nUsedChunks - 1;
    }

private:
    // Grow the chunk map by 2x. Re-centre the existing chunk pointers.
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

    // Increment given chunk map position
    void inc(uint32_t& chunk, uint32_t& off) {
        if (off == chunkSize - 1) {
            chunk++;
            off = 0;
        } else {
            off++;    
        }

        // lazily allocate new chunk (if needed)
        if (0 <= chunk && chunk < nChunks && chunkMap[chunk] == nullptr) {
            chunkMap[chunk] = elementAllocator.allocate(chunkSize);
        }
    }

    // Decrement given chunk map position
    void dec(uint32_t& chunk, uint32_t& off) {
        if (off == 0) {
            chunk--;
            off = chunkSize - 1;
        } else {
            off--;
        }

        // lazily allocate new chunk (if needed)
        if (0 <= chunk && chunk < nChunks && chunkMap[chunk] == nullptr) {
            chunkMap[chunk] = elementAllocator.allocate(chunkSize);
        }
    }

    // 
    // Shift all elements in range [start, end] (inclusive) one position to the left.
    //
    void shiftLeft(uint32_t startChunk, uint32_t startOff, uint32_t endChunk, uint32_t endOff) {
        uint32_t currChunk = startChunk;
        uint32_t currOff = startOff;
        
        // start one to the left of the start pointer
        dec(currChunk, currOff);

        // walk forwards, moving each element left one position
        while (!(currChunk == endChunk && currOff == endOff)) {
            if (currOff == chunkSize - 1) {
                chunkMap[currChunk][currOff] = std::move(chunkMap[currChunk + 1][0]);
                currChunk += 1;
                currOff = 0;
            } else {
                chunkMap[currChunk][currOff] = std::move(chunkMap[currChunk][currOff + 1]);
                currOff += 1;
            }
        }

        chunkMap[endChunk][endOff] = T();
    }

    //
    // Shift all elements in range [start, end] (inclusive) one position to the right
    //
    void shiftRight(uint32_t startChunk, uint32_t startOff, uint32_t endChunk, uint32_t endOff) {
        uint32_t currChunk = endChunk;
        uint32_t currOff = endOff;

        // start one to the right of the end pointer
        inc(currChunk, currOff);

        // walk backwards, moving each element right one position
        while (!(startChunk == currChunk && startOff == currOff)) {
            if (currOff > 0) {
                chunkMap[currChunk][currOff] = std::move(chunkMap[currChunk][currOff - 1]);
                currOff -= 1;
            } else {
                chunkMap[currChunk][currOff] = std::move(chunkMap[currChunk - 1][chunkSize - 1]);
                currChunk -= 1;
                currOff = chunkSize - 1;
            }
        }

        chunkMap[startChunk][startOff] = T();
    }
};
};