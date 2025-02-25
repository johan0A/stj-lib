#pragma once

#include <iostream>
#include <cstdint>
#include <cstdlib>

using u8 = std::uint8_t;
using i8 = std::int8_t;

using u16 = std::uint16_t;
using i16 = std::int16_t;

using u32 = std::uint32_t;
using i32 = std::int32_t;

using u64 = std::uint64_t;
using i64 = std::int64_t;

using usize = std::size_t;
using isize = std::ptrdiff_t;

// panic ===
#define PANIC(msg)                                   \
    do {                                             \
        std::cerr << "Panic at " << __FILE__ << ":"  \
                  << __LINE__ << ": " << msg << "\n";\
        std::abort();                                \
    } while(0)
// === panic

// defer ====
template <typename F>
struct privDefer {
	F f;
	privDefer(F f) : f(f) {}
	~privDefer() { f(); }
};

template <typename F>
privDefer<F> defer_func(F f) {
	return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})
// ==== defer

// single item ptr ====
template <typename T>
class Ptr {
public:
    T* raw_ptr;

    Ptr(T* p) : raw_ptr(p) {
        if (p == nullptr) {
            PANIC("Cannot initialize Ptr with nullptr");
        }
    }

    T& v() const {
        return *raw_ptr;
    }
};
// ==== single item ptr

template <typename T> class MiPtr;
template <typename T> struct Slice;

// many items pointer ====
template <typename T>
class MiPtr {
public:
    T* raw_ptr;

    MiPtr(T* p) : raw_ptr(p) {
        if (p == nullptr) [[unlikely]] {
            PANIC("Cannot initialize MiPtr with nullptr");
        }
    }
    
    T& operator[] (usize index) {
        return raw_ptr[index];
    }

    Slice<T> slice(usize start, usize end) {
        if (start > end) [[unlikely]] {
            PANIC("start is greater than end");
        }
        MiPtr<T> shifted_ptr = raw_ptr + start;
        return Slice<T>{shifted_ptr, end - start};
    }

    MiPtr<T> slice(usize start) {
        return MiPtr<T>(raw_ptr + start);
    }
};
// ==== many items pointer

// Slice ====
template <typename T>
struct Slice {
    MiPtr<T> ptr;
    usize len;

    T& operator[] (usize index) {
        if (index >= len) [[unlikely]] {
            PANIC("index is greater than slice bound");
        }
        return ptr[index];
    }

    static Slice empty() {
        static T dummy_value; // TODO: Yuck?
        return Slice{MiPtr<T>(&dummy_value), 0};
    }

    Slice<T> slice(usize start, usize end) {
        if (end >= len) [[unlikely]] {
            PANIC("end is greater than slice bound");
        }
        if (start > end) [[unlikely]] {
            PANIC("start is greater than end");
        }
        MiPtr<T> shifted_ptr = ptr.slice(start);
        return Slice<T>{shifted_ptr, end - start};
    }

    Slice<T> slice(usize start) {
        if (start >= len) [[unlikely]] {
            PANIC("start is greater than slice bound");
        }
        MiPtr<T> shifted_ptr = ptr.slice(start);
        return Slice<T>{shifted_ptr, len - start};
    }
};
// ==== Slice