# rackstl
A from-scratch implementation of C++'s core standard library (STL).

### Key features
- Core data structures
  - vector, map, set, unordered_map, unordered_set, deque, queue, priority_queue
- Smart pointers
  - shared_ptr, unique_ptr, weak_ptr + shared_from_this
- Benchmarking
  - Each implementation benchmarked against STL corrolary
- Testing
  - GTest used, each implementation given own suite of comprehensive unit tests

### Usage
Implementations are header-only and completely self-contained. Simply include, and you're off and running.

### Run benchmarks / tests
```bash
mkdir build
cd build
cmake ..
make

./bench     ## benchmarks
./test      ## tests
```