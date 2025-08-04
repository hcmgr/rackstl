namespace rack {

template <class T>
class shared_ptr {
private:

    // control block shared by each shared_ptr referencing `ptr`
    struct SharedPtrControlBlock {

        uint32_t strongCnt;
        uint32_t weakCnt;

        SharedPtrControlBlock()
            : strongCnt(0), weakCnt(0) {}
    };

    T* ptr;
    SharedPtrControlBlock* controlBlock;

public:

    //////////////////////////////////////////////////////
    // Constructors
    //////////////////////////////////////////////////////

    shared_ptr() {
        ptr = nullptr;
        controlBlock = nullptr;
    }

    shared_ptr(T* p) {
        ptr = p;
        controlBlock = new SharedPtrControlBlock();
        controlBlock->strongCnt++;
    }

    ~shared_ptr() {
        release();
    }

    // Copy constructor
    shared_ptr(const shared_ptr& other) {
        ptr = other.ptr;
        controlBlock = other.controlBlock;
        controlBlock->strongCnt++;
    }

    // Move constructor
    shared_ptr(const shared_ptr&& other) {

    }

    // Copy assignment
    shared_ptr<T>& operator=(const shared_ptr<T>& other) {
        reset();

        ptr = other.ptr;
        controlBlock = other.controlBlock;
        if (controlBlock) { // `other` could be a null shared_ptr (perfectly valid)
            controlBlock->strongCnt++;
        }

        return *this;
    }

    // TODO: move assignment

    //////////////////////////////////////////////////////
    // Modifiers
    //////////////////////////////////////////////////////

    void reset() {
        release();
        ptr = nullptr;
        controlBlock = nullptr;
    }

    void reset(T* newPtr) {
        release();
        if (newPtr) {
            ptr = newPtr;
            controlBlock = new SharedPtrControlBlock();
            controlBlock->strongCnt++;
        }
    }

    // Swap pointers to managed object with `other`.
    void swap(shared_ptr& other) {
        std::swap(ptr, other.ptr);
    }

    //////////////////////////////////////////////////////
    // Observers
    //////////////////////////////////////////////////////

    T* get() {
        return ptr;
    }

    T& operator*() const { return *ptr; }
    T* operator->()  const { return ptr; }

    uint32_t use_count() {
        if (controlBlock == nullptr) {
            return 0;
        }
        return controlBlock->strongCnt;
    }

    bool unique() {
        return controlBlock && controlBlock->strongCnt == 1;
    }

    operator bool() const {
        return ptr != nullptr;
    }

private:

    //
    // Releases ownership of the managed object by:
    //      - decrementing strong refnct AND;
    //      - if applicable, de-allocating underlying memory and/or control block
    //
    void release() {
        // already released
        if (controlBlock == nullptr) { 
            return; 
        }

        controlBlock->strongCnt--;
        if (controlBlock->strongCnt == 0) {

            // no owning references left - free managed object
            delete ptr;

            // also no non-owning references left - free control block
            if (controlBlock->weakCnt == 0) {
                delete controlBlock;
            }
        }
    }
};

template <class T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
    return shared_ptr<T>(new T(std::forward<Args>(args)...));
}

}; // end of 'rack'
