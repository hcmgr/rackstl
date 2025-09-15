# rackstl
A from-scratch implementation of C++'s core standard library (STL).

### Key features
- Core data structures
  - vector, map, set, unordered_map, unordered_set, deque, queue, priority_queue
- Smart pointers
  - shared_ptr, unique_ptr, weak_ptr
- Concurrency primitives
  - thread, lock_guard

### Build
```bash
mkdir build
cd build
cmake ..
make
./test

```