#pragma once

#include <iostream>
#include <cstdint>

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
#define PANIC(msg) do { \
    std::cerr << "Panic at " << __FILE__ << ":" << __LINE__ << ": " << msg << " \n"; \
    std::abort(); \
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

// Error ====
template <typename... Enums>
class Error {
private:
    template <typename D, typename... Ts>
    struct contains_type {
        static constexpr bool value = (std::is_same_v<D, Ts> || ...);
    };

    template <typename D, typename... Ts>
    struct index_of;

    template <typename D, typename First, typename... Rest>
    struct index_of<D, First, Rest...> {
        static constexpr std::size_t value = std::is_same_v<D, First> ? 0 : 1 + index_of<D, Rest...>::value;
    };

    template <typename D>
    struct index_of<D> {
        static constexpr std::size_t value = 0;
    };

    using TagType = std::uint8_t;
    static constexpr TagType INVALID_TAG = 0;
    static constexpr TagType VALUE_TAG = 1;
    
    union Storage {
        char errorStorage[std::max({sizeof(Enums)...})];
    } storage;
    
    TagType tag;
    
    template <typename E>
    static constexpr TagType getTagForType() {
        static_assert(
            contains_type<E, Enums...>::value, 
            "Type not in the list of supported enum types"
        );
        return static_cast<TagType>(index_of<E, Enums...>::value + 2);
    }

    template <typename E, typename... Os>
    void tryConvertEnum(const Error<Os...>& other, bool& converted) {
        if (!converted && other.template is<E>()) {
            E error = other.template get<E>();
            tag = getTagForType<E>();
            *reinterpret_cast<E*>(storage.errorStorage) = error;
            converted = true;
        }
    }
public:
    Error() : tag(INVALID_TAG) {}
    
    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    Error(E error) : tag(getTagForType<E>()) {
        *reinterpret_cast<E*>(storage.errorStorage) = error;
    }
    
    template <typename... OtherEnums>
    Error(const Error<OtherEnums...>& other) : tag(INVALID_TAG) {
        static_assert(
            (contains_type<OtherEnums, Enums...>::value && ...), 
            "Target Error type must include all enum types from source Error"
        );

        if (other.isEmpty()) {
            return;
        }

        bool converted = false;
        (tryConvertEnum<OtherEnums>(other, converted), ...);
    }

    // Check if contains specific error
    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    bool is() const {
        return tag == getTagForType<E>();
    }
    
    // Check if contains any error
    bool hasError() const {
        return tag != INVALID_TAG;
    }
    
    // Get error
    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    E get() const {
        if (tag != getTagForType<E>()) {
            PANIC("Error does not contain this error type");
        }
        return *reinterpret_cast<const E*>(storage.errorStorage);
    }
    
    // Check if empty
    bool isEmpty() const {
        return tag == INVALID_TAG;
    }
};
// ==== Error

// Result ====
template <typename T, typename... Enums>
class Result {
    private:
    template <typename D, typename... Ts>
    struct contains_type {
        static constexpr bool value = (std::is_same_v<D, Ts> || ...);
    };

    template <typename D, typename... Ts>
    struct index_of;

    template <typename D, typename First, typename... Rest>
    struct index_of<D, First, Rest...> {
        static constexpr std::size_t value = std::is_same_v<D, First> ? 0 : 1 + index_of<D, Rest...>::value;
    };

    template <typename D>
    struct index_of<D> {
        static constexpr std::size_t value = 0;
    };

    using TagType = std::uint8_t;
    static constexpr TagType INVALID_TAG = 0;
    static constexpr TagType VALUE_TAG = 1;
    
    union Storage {
        T value;
        char errorStorage[std::max({sizeof(Enums)...})];
    } storage;
    
    TagType tag;
    
    template <typename E>
    static constexpr TagType getTagForType() {
        static_assert(
            contains_type<E, Enums...>::value, 
            "Type not in the list of supported enum types"
        );
        return static_cast<TagType>(index_of<E, Enums...>::value + 2);
    }

    template <typename E, typename U, typename... Os>
    void tryConvertEnum(const Result<U, Os...>& other, bool& converted) {
        if (!converted && other.template hasError<E>()) {
            E error = other.template error<E>();
            tag = getTagForType<E>();
            *reinterpret_cast<E*>(storage.errorStorage) = error;
            converted = true;
        }
    }
public:
    Result() : tag(INVALID_TAG) {}
    
    Result(const T value) : tag(VALUE_TAG) {
        storage.value = value;
    }
    
    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    Result(E error) : tag(getTagForType<E>()) {
        *reinterpret_cast<E*>(storage.errorStorage) = error;
    }

    template <typename U, typename... OtherEnums, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
    Result(const Result<U, OtherEnums...>& other) : tag(INVALID_TAG) {
        static_assert(
            (contains_type<OtherEnums, Enums...>::value && ...), 
            "Target Result type must include all enum types from source Result"
        );
        
        if (other.isEmpty()) {
            return;
        }
        
        if (other.hasValue()) {
            tag = VALUE_TAG;
            storage.value = other.value();
            return;
        }
        
        bool converted = false;
        (tryConvertEnum<OtherEnums>(other, converted), ...);
    }
    
    // Check if contains value
    bool hasValue() const {
        return tag == VALUE_TAG;
    }
    
    // Check if contains specific error
    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    bool hasError() const {
        return tag == getTagForType<E>();
    }
    
    // Check if contains any error
    bool hasAnyError() const {
        return tag > VALUE_TAG;
    }
    
    // Get value
    T& value() {
        if (tag != VALUE_TAG) {
            PANIC("Result does not contain a value");
        }
        return storage.value;
    }
    
    const T& value() const {
        if (tag != VALUE_TAG) {
            PANIC("Result does not contain a value");
        }
        
        return storage.value;
    }
    
    // Get error
    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    E error() const {
        if (tag != getTagForType<E>()) {
            PANIC("Result does not contain this error type");
        }
        return *reinterpret_cast<const E*>(storage.errorStorage);
    }
    
    // Get value or default
    T valueOr(const T& defaultValue) const {
        return hasValue() ? value() : defaultValue;
    }
    
    // Check if empty
    bool isEmpty() const {
        return tag == INVALID_TAG;
    }
};
// ==== Result

// errdefer ====
#define errscope bool stj__error = false;

// TODO: check for the type of _result_temp
#define TRY(expr) ({ \
    auto _result_temp = (expr); \
    if (_result_temp.hasAnyError()) { \
        stj__error = true; \
        return _result_temp; \
    } \
    _result_temp.value(); \
})

template <typename F>
struct privErrdefer {
	F f;
	privErrdefer(F f) : f(f) {}
	~privErrdefer() { f(); }
};

template <typename F>
privDefer<F> errdefer_func(F f) {
	return privDefer<F>(f);
}

#define ERRDEFER_1(x, y) x##y
#define ERRDEFER_2(x, y) ERRDEFER_1(x, y)
#define ERRDEFER_3(x)    ERRDEFER_2(x, __COUNTER__)
#define errdefer(code) auto ERRDEFER_3(_defer_) = errdefer_func([&](){ \
    if (stj__error) {code;} \
});
// ==== errdefer

// single item ptr ====
template <typename T>
class Ptr {
private:
    struct PrivateTag {};
    Ptr(T* p, PrivateTag) : raw_ptr(p) {} // workaround for creating a Ptr with a nullptr for undefined
public:
    // TODO support void pointer
    T* raw_ptr;
    
    Ptr(T* p) : raw_ptr(p) {
        if (p == nullptr) {
            PANIC("Cannot initialize Ptr with nullptr");
        }
    }
    
    static Ptr<T> undefined() {
        return Ptr<T>(nullptr, PrivateTag{});
    }

    T& v() const {
        return *raw_ptr;
    }
    
    template <typename... Args>
    auto operator()(Args&&... args) const -> decltype((*raw_ptr)(std::forward<Args>(args)...)) {
        return (*raw_ptr)(std::forward<Args>(args)...);
    }
};
// ==== single item ptr

template <typename T> class MiPtr;
template <typename T> struct Slice;

// many items pointer ====
template <typename T>
class MiPtr {
private:
    struct PrivateTag {};
    MiPtr(T* p, PrivateTag) : raw_ptr(p) {} // workaround for creating a Ptr with a nullptr for undefined
public:
    T* raw_ptr;

    MiPtr(T* p) : raw_ptr(p) {
        if (p == nullptr) [[unlikely]] {
            PANIC("Cannot initialize MiPtr with nullptr");
        }
    }

    static MiPtr<T> undefined() {
        return MiPtr<T>(nullptr, PrivateTag{});
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
        return Slice{MiPtr<T>::undefined(), 0};
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