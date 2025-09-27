#include <memory>
#include <iostream>
#pragma once

template <typename... Args>
inline void MyLogImpl(const char* func, Args&&... args) {
    std::cout << "[" << func << "] ";
    (std::cout << ... << args) << "\n";
}

#define MyLog(...) MyLogImpl(__func__, __VA_ARGS__)

namespace utils {
    template <typename Alloc, typename T, typename... Args>
    void allocConstruct(Alloc& alloc, T* p, Args&&... args) {
        std::allocator_traits<Alloc>::construct(
            alloc, p, std::forward<Args>(args)...
        );
    }

    template <typename Alloc, typename T>
    void allocDestroy(Alloc& alloc, T* p) {
        std::allocator_traits<Alloc>::destroy(alloc, p);
    }
}