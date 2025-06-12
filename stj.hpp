#pragma once


// TODO: remove
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <type_traits>
#include <limits> 
#include <algorithm>
#include <utility>

// TODO: clean the namespace
namespace __stj_basic_impl {
    thread_local bool error = false; 
}

// panic ===
#define PANIC(msg) do { \
    std::fprintf(stderr, "Panic at %s:%d: %s\n", __FILE__, __LINE__, msg); \
    std::abort(); \
} while(0)
// === panic

// SafeInt ====
template <typename T>
class SafeInt {
private:
    static_assert(std::is_integral_v<T>, "SafeInt can only wrap integral types");
    T value;

    static bool add_overflow(T a, T b, T& result) {
        if constexpr (std::is_unsigned_v<T>) {
            result = a + b;
            return result < a;
        } else {
            if (b > 0 && a > std::numeric_limits<T>::max() - b) return true;
            if (b < 0 && a < std::numeric_limits<T>::min() - b) return true;
            result = a + b;
            return false;
        }
    }

    static bool sub_overflow(T a, T b, T& result) {
        if constexpr (std::is_unsigned_v<T>) {
            result = a - b;
            return a < b;
        } else {
            if (b < 0 && a > std::numeric_limits<T>::max() + b) return true;
            if (b > 0 && a < std::numeric_limits<T>::min() + b) return true;
            result = a - b;
            return false;
        }
    }

    static bool mul_overflow(T a, T b, T& result) {
        if constexpr (std::is_unsigned_v<T>) {
            if (a == 0 || b == 0) {
                result = 0;
                return false;
            }
            result = a * b;
            return result / a != b;
        } else {
            if (a == 0 || b == 0) {
                result = 0;
                return false;
            }
            if (a == -1 && b == std::numeric_limits<T>::min()) return true;
            if (b == -1 && a == std::numeric_limits<T>::min()) return true;
            
            T max = std::numeric_limits<T>::max();
            T min = std::numeric_limits<T>::min();
            
            if (a > 0 && b > 0 && a > max / b) return true;
            if (a < 0 && b < 0 && a < max / b) return true;
            if (a > 0 && b < 0 && b < min / a) return true;
            if (a < 0 && b > 0 && a < min / b) return true;
            
            result = a * b;
            return false;
        }
    }

public:
    SafeInt() : value(0) {}
    SafeInt(T val) : value(val) {}

    template <typename U, typename = std::enable_if_t<std::is_integral_v<U>>>
    SafeInt(U val) : value(static_cast<T>(val)) {
        if (static_cast<U>(static_cast<T>(val)) != val) {
            PANIC("Integer conversion overflow");
        }
    }

    template <typename U>
    SafeInt(const SafeInt<U>& other) : value(static_cast<T>(other.raw())) {
        if (static_cast<U>(static_cast<T>(other.raw())) != other.raw()) {
            PANIC("Integer conversion overflow");
        }
    }

    operator T() const { return value; }

    T raw() const { return value; }

    SafeInt operator+(const SafeInt& other) const {
        T result;
        if (add_overflow(value, other.value, result)) {
            PANIC("Integer overflow in addition");
        }
        return SafeInt(result);
    }

    SafeInt operator-(const SafeInt& other) const {
        T result;
        if (sub_overflow(value, other.value, result)) {
            PANIC("Integer underflow in subtraction");
        }
        return SafeInt(result);
    }

    SafeInt operator*(const SafeInt& other) const {
        T result;
        if (mul_overflow(value, other.value, result)) {
            PANIC("Integer overflow in multiplication");
        }
        return SafeInt(result);
    }

    SafeInt operator/(const SafeInt& other) const {
        if (other.value == 0) {
            PANIC("Division by zero");
        }
        if constexpr (std::is_signed_v<T>) {
            if (value == std::numeric_limits<T>::min() && other.value == -1) {
                PANIC("Integer overflow in division");
            }
        }
        return SafeInt(value / other.value);
    }

    SafeInt operator%(const SafeInt& other) const {
        if (other.value == 0) {
            PANIC("Modulo by zero");
        }
        if constexpr (std::is_signed_v<T>) {
            if (value == std::numeric_limits<T>::min() && other.value == -1) {
                return SafeInt(0);
            }
        }
        return SafeInt(value % other.value);
    }

    template <typename U>
    auto operator+(const U& other) const -> SafeInt<decltype(value + U{})> {
        using ResultType = decltype(value + U{});
        return SafeInt<ResultType>(value) + SafeInt<ResultType>(other);
    }

    template <typename U>
    auto operator-(const U& other) const -> SafeInt<decltype(value - U{})> {
        using ResultType = decltype(value - U{});
        return SafeInt<ResultType>(value) - SafeInt<ResultType>(other);
    }

    template <typename U>
    auto operator*(const U& other) const -> SafeInt<decltype(value * U{})> {
        using ResultType = decltype(value * U{});
        return SafeInt<ResultType>(value) * SafeInt<ResultType>(other);
    }

    template <typename U>
    auto operator/(const U& other) const -> SafeInt<decltype(value / U{})> {
        using ResultType = decltype(value / U{});
        return SafeInt<ResultType>(value) / SafeInt<ResultType>(other);
    }

    template <typename U>
    auto operator%(const U& other) const -> SafeInt<decltype(value % U{})> {
        using ResultType = decltype(value % U{});
        return SafeInt<ResultType>(value) % SafeInt<ResultType>(other);
    }

    SafeInt operator-() const {
        if constexpr (std::is_signed_v<T>) {
            if (value == std::numeric_limits<T>::min()) {
                PANIC("Integer overflow in negation");
            }
            return SafeInt(-value);
        } else {
            PANIC("Cannot negate unsigned integer");
        }
    }

    SafeInt& operator+=(const SafeInt& other) {
        T result;
        if (add_overflow(value, other.value, result)) {
            PANIC("Integer overflow in addition");
        }
        value = result;
        return *this;
    }

    SafeInt& operator-=(const SafeInt& other) {
        T result;
        if (sub_overflow(value, other.value, result)) {
            PANIC("Integer underflow in subtraction");
        }
        value = result;
        return *this;
    }

    SafeInt& operator*=(const SafeInt& other) {
        T result;
        if (mul_overflow(value, other.value, result)) {
            PANIC("Integer overflow in multiplication");
        }
        value = result;
        return *this;
    }

    SafeInt& operator/=(const SafeInt& other) {
        *this = *this / other;
        return *this;
    }

    SafeInt& operator%=(const SafeInt& other) {
        *this = *this % other;
        return *this;
    }

    template <typename U>
    SafeInt& operator+=(const U& other) {
        *this = SafeInt(*this + other);
        return *this;
    }

    template <typename U>
    SafeInt& operator-=(const U& other) {
        *this = SafeInt(*this - other);
        return *this;
    }

    template <typename U>
    SafeInt& operator*=(const U& other) {
        *this = SafeInt(*this * other);
        return *this;
    }

    template <typename U>
    SafeInt& operator/=(const U& other) {
        *this = SafeInt(*this / other);
        return *this;
    }

    template <typename U>
    SafeInt& operator%=(const U& other) {
        *this = SafeInt(*this % other);
        return *this;
    }

    template <typename U>
    SafeInt& operator=(const U& other) {
        *this = SafeInt(other);
        return *this;
    }

    SafeInt& operator++() {
        T result;
        if (add_overflow(value, T(1), result)) {
            PANIC("Integer overflow in increment");
        }
        value = result;
        return *this;
    }

    SafeInt operator++(int) {
        SafeInt old = *this;
        ++(*this);
        return old;
    }

    SafeInt& operator--() {
        T result;
        if (sub_overflow(value, T(1), result)) {
            PANIC("Integer underflow in decrement");
        }
        value = result;
        return *this;
    }

    SafeInt operator--(int) {
        SafeInt old = *this;
        --(*this);
        return old;
    }

    bool operator==(const SafeInt& other) const { return value == other.value; }
    bool operator!=(const SafeInt& other) const { return value != other.value; }
    bool operator<(const SafeInt& other) const { return value < other.value; }
    bool operator<=(const SafeInt& other) const { return value <= other.value; }
    bool operator>(const SafeInt& other) const { return value > other.value; }
    bool operator>=(const SafeInt& other) const { return value >= other.value; }

    template <typename U>
    bool operator==(const U& other) const { return value == other; }
    
    template <typename U>
    bool operator!=(const U& other) const { return value != other; }
    
    template <typename U>
    bool operator<(const U& other) const { return value < other; }
    
    template <typename U>
    bool operator<=(const U& other) const { return value <= other; }
    
    template <typename U>
    bool operator>(const U& other) const { return value > other; }
    
    template <typename U>
    bool operator>=(const U& other) const { return value >= other; }

    SafeInt operator&(const SafeInt& other) const { return SafeInt(value & other.value); }
    SafeInt operator|(const SafeInt& other) const { return SafeInt(value | other.value); }
    SafeInt operator^(const SafeInt& other) const { return SafeInt(value ^ other.value); }
    SafeInt operator~() const { return SafeInt(~value); }
    
    SafeInt operator<<(const SafeInt& shift) const {
        if (shift.value < 0 || shift.value >= static_cast<T>(sizeof(T) * 8)) {
            PANIC("Invalid shift amount");
        }
        if (shift.value > 0) {
            T max_shift = static_cast<T>(sizeof(T) * 8 - 1);
            if constexpr (std::is_unsigned_v<T>) {
                if (value > (std::numeric_limits<T>::max() >> shift.value)) {
                    PANIC("Integer overflow in left shift");
                }
            } else {
                if (value > 0 && value > (std::numeric_limits<T>::max() >> shift.value)) {
                    PANIC("Integer overflow in left shift");
                }
                if (value < 0 && value < (std::numeric_limits<T>::min() >> shift.value)) {
                    PANIC("Integer underflow in left shift");
                }
            }
        }
        return SafeInt(value << shift.value);
    }
    
    SafeInt operator>>(const SafeInt& shift) const {
        if (shift.value < 0 || shift.value >= static_cast<T>(sizeof(T) * 8)) {
            PANIC("Invalid shift amount");
        }
        return SafeInt(value >> shift.value);
    }
};

using u8 = SafeInt<std::uint8_t>;
using i8 = SafeInt<std::int8_t>;

using u16 = SafeInt<std::uint16_t>;
using i16 = SafeInt<std::int16_t>;

using u32 = SafeInt<std::uint32_t>;
using i32 = SafeInt<std::int32_t>;

using u64 = SafeInt<std::uint64_t>;
using i64 = SafeInt<std::int64_t>;

using usize = SafeInt<std::size_t>;
using isize = SafeInt<std::ptrdiff_t>;
// ==== SafeInt

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

    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    bool is() const {
        return tag == getTagForType<E>();
    }
    
    bool hasError() const {
        return tag != INVALID_TAG;
    }
    
    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    E get() const {
        if (tag != getTagForType<E>()) {
            PANIC("Error does not contain this error type");
        }
        return *reinterpret_cast<const E*>(storage.errorStorage);
    }
    
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
        alignas(T) char valueStorage[sizeof(T)];
        char errorStorage[std::max({sizeof(Enums)...})];
        
        Storage() {}
        ~Storage() {}
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

    void cleanup() {
        if (tag == VALUE_TAG) {
            reinterpret_cast<T*>(storage.valueStorage)->~T();
        }
    }

    void copyFrom(const Result& other) {
        tag = other.tag;
        if (tag == VALUE_TAG) {
            new (storage.valueStorage) T(other.value());
        } else if (tag > VALUE_TAG) {
            std::memcpy(storage.errorStorage, other.storage.errorStorage, sizeof(storage.errorStorage));
        }
    }

    void moveFrom(Result&& other) {
        tag = other.tag;
        if (tag == VALUE_TAG) {
            new (storage.valueStorage) T(std::move(other.value()));
        } else if (tag > VALUE_TAG) {
            std::memcpy(storage.errorStorage, other.storage.errorStorage, sizeof(storage.errorStorage));
        }
        other.tag = INVALID_TAG;
    }

public:
    Result() : tag(INVALID_TAG) {}
    
    Result(const T& value) : tag(VALUE_TAG) {
        new (storage.valueStorage) T(value);
    }
    
    Result(T&& value) : tag(VALUE_TAG) {
        new (storage.valueStorage) T(std::move(value));
    }
    
    template <typename U, typename = std::enable_if_t<
        std::is_convertible_v<U, T> && 
        !std::is_same_v<std::decay_t<U>, T> &&
        !contains_type<std::decay_t<U>, Enums...>::value
    >>
    Result(U&& value) : tag(VALUE_TAG) {
        new (storage.valueStorage) T(std::forward<U>(value));
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
            new (storage.valueStorage) T(other.value());
            return;
        }
        
        bool converted = false;
        (tryConvertEnum<OtherEnums>(other, converted), ...);
    }
    
    Result(const Result& other) : tag(INVALID_TAG) {
        copyFrom(other);
    }
    
    Result(Result&& other) noexcept : tag(INVALID_TAG) {
        moveFrom(std::move(other));
    }
    
    Result& operator=(const Result& other) {
        if (this != &other) {
            cleanup();
            copyFrom(other);
        }
        return *this;
    }
    
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            cleanup();
            moveFrom(std::move(other));
        }
        return *this;
    }
    
    ~Result() {
        cleanup();
    }
    
    bool hasValue() const {
        return tag == VALUE_TAG;
    }
    
    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    bool hasError() const {
        return tag == getTagForType<E>();
    }
    
    bool hasAnyError() const {
        return tag > VALUE_TAG;
    }
    
    T& value() {
        if (tag != VALUE_TAG) {
            PANIC("Result does not contain a value");
        }
        return *reinterpret_cast<T*>(storage.valueStorage);
    }
    
    const T& value() const {
        if (tag != VALUE_TAG) {
            PANIC("Result does not contain a value");
        }
        return *reinterpret_cast<const T*>(storage.valueStorage);
    }
    
    template <typename E, typename = std::enable_if_t<contains_type<E, Enums...>::value>>
    E error() const {
        if (tag != getTagForType<E>()) {
            PANIC("Result does not contain this error type");
        }
        return *reinterpret_cast<const E*>(storage.errorStorage);
    }
    
    T valueOr(const T& defaultValue) const {
        return hasValue() ? value() : defaultValue;
    }
    
    bool isEmpty() const {
        return tag == INVALID_TAG;
    }
};
// ==== Result

// errdefer ====
// TODO: check for the type of _result_temp
#define TRY(expr) ({ \
    auto _result_temp = (expr); \
    __stj_basic_impl::error = _result_temp.hasAnyError(); \
    if (__stj_basic_impl::error) { \
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

// TODO: move stuff to a namepsace
#define ERRDEFER_1(x, y) x##y
#define ERRDEFER_2(x, y) ERRDEFER_1(x, y)
#define ERRDEFER_3(x)    ERRDEFER_2(x, __COUNTER__)
#define errdefer(code) auto ERRDEFER_3(_defer_) = errdefer_func([&](){ \
    if (__stj_basic_impl::error) {code;} \
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

namespace stj {
    namespace heap {
        struct AllocatorVTable {
            /// Attempt to allocate exactly `len` bytes.
            ///
            Ptr<Slice<u8>(void* ctx, usize len)> alloc;

            /// Attempt to expand or shrink memory in place. `buf.len` must equal the
            /// length requested from the most recent successful call to `alloc` or
            /// `resize`.
            ///
            /// A result of `true` indicates the resize was successful and the
            /// allocation now has the same address but a size of `new_len`. `false`
            /// indicates the resize could not be completed without moving the
            /// allocation to a different address and thus alloc should be used to 
            /// complete the resize
            ///
            /// `new_len` must be greater than zero.
            ///
            Ptr<bool(void* ctx, Slice<u8> buf, usize new_len)> resize;

            /// Free and invalidate a buffer.
            ///
            /// `buf.len` must equal the most recent length returned by `alloc` or
            /// given to a successful `resize` call.
            ///
            Ptr<void(void* ctx, Slice<u8> buf)> free;
        };

        // Allocator interface
        struct Allocator {
            void* impl_data;
            const AllocatorVTable* vtable;

            // Allocate a slice of bytes of given capacity
            template <typename T>
            Slice<T> alloc(usize count) {
                usize byte_size = count * sizeof(T);
                Slice<u8> bytes = vtable->alloc(impl_data, byte_size);
                if (bytes.len == 0) [[unlikely]] {
                    PANIC("Memory allocation failed");
                }
                return Slice<T>{MiPtr<T>(reinterpret_cast<T*>(bytes.ptr.raw_ptr)), count};
            }

            // Deallocate a previously allocated slice
            template <typename T>
            void free(Slice<T> slice) {
                Slice<u8> bytes{
                    MiPtr<u8>(reinterpret_cast<u8*>(slice.ptr.raw_ptr)), // TODO: make a cast function for Ptr<>?
                    slice.len * sizeof(T)
                };
                vtable->free(impl_data, bytes);
            }
            
            // Create a single item
            template <typename T>
            Ptr<T> create() {
                Slice<T> memory = alloc<T>(1);
                return memory.ptr;
            }
            
            // Destroy a single item
            template <typename T>
            void destroy(Ptr<T> item) {
                Slice<u8> bytes{
                    MiPtr<u8>(reinterpret_cast<u8*>(item.get_raw_ptr)), 
                    sizeof(T)
                };
                vtable->free(impl_data, bytes);
            }

            // Resize an existing allocation
            template <typename T>
            Slice<T> realloc(Slice<T> old_slice, usize new_count) {
                usize new_byte_size = new_count * sizeof(T);
                
                Slice<u8> bytes{
                    MiPtr<u8>(reinterpret_cast<u8*>(old_slice.ptr.raw_ptr)), 
                    old_slice.len * sizeof(T)
                };
                
                bool success = vtable->resize(impl_data, bytes, new_byte_size);
                
                if (success) {
                    return Slice<T>{old_slice.ptr, new_count};
                } else {
                    Slice<T> new_slice = alloc<T>(new_count);
                    
                    usize copy_count; 
                    if (old_slice.len < new_count) {
                        copy_count = old_slice.len;
                    } else {
                        copy_count = new_count;
                    }

                    for (usize i = 0; i < copy_count; i++) {
                        new_slice[i] = old_slice[i];
                    }
                    
                    free(old_slice);
                    
                    return new_slice;
                }
            }
        };
        
        namespace c_allocator_impl {
            namespace extern_c {
                #if defined(_WIN32) || defined(_WIN64)
                    #if defined(myDLL_EXPORTS)
                        extern "C" std::size_t __declspec(dllexport) _msize(void* memblock);
                    #else
                        extern "C" std::size_t __declspec(dllimport) _msize(void* memblock);
                    #endif
                #elif defined(__APPLE__)
                    extern "C" std::size_t malloc_size(const void* ptr);
                #elif defined(__FreeBSD__) || defined(__linux__)
                    extern "C" std::size_t malloc_usable_size(const void* ptr);
                #endif
            }
        
            inline std::size_t get_allocation_size(void* ptr) {
                if (ptr == nullptr) return 0;
                
                #if defined(_WIN32) || defined(_WIN64)
                    return extern_c::_msize(const_cast<void*>(ptr));
                #elif defined(__APPLE__)
                    return extern_c::malloc_size(ptr);
                #elif defined(__FreeBSD__) || defined(__linux__)
                    return extern_c::malloc_usable_size(ptr);
                #else
                    return 0;
                #endif
            }

            static Slice<u8> malloc_alloc(void* /*ctx*/, usize size) {
                if (size == 0) return {MiPtr<u8>(nullptr), 0};
                
                u8* ptr = static_cast<u8*>(::malloc(size));
                if (ptr == nullptr) [[unlikely]] {
                    return {MiPtr<u8>(nullptr), 0};
                }
                
                return {MiPtr<u8>(ptr), size};
            }
    
            static bool malloc_resize(void* /*ctx*/, Slice<u8> buf, usize new_size) {
                return new_size <= get_allocation_size(buf.ptr.raw_ptr);
            }
    
            static void malloc_free(void* /*ctx*/, Slice<u8> buf) {
                ::free(buf.ptr.raw_ptr);
            }
    
            static const AllocatorVTable malloc_vtable = {
                malloc_alloc,
                malloc_resize,
                malloc_free
            };
        }
        
        const Allocator c_allocator = {
            nullptr,
            &c_allocator_impl::malloc_vtable
        };
    }

    template <typename T>
    struct ArrayList {
        Slice<T> items;
        usize capacity;

        static ArrayList<T> init() {
            return { 
                .items = Slice<T>::empty(),
                .capacity = 0 
            };
        }

        void deinit(heap::Allocator alloc) {
            alloc.free(items.ptr.slice(0, capacity));
        }

        void append(heap::Allocator alloc, T item) {
            if (capacity == 0) {
                capacity = 16;
                Slice<T> buff = alloc.alloc<T>(capacity);
                items = buff.slice(0, 1);
                items[0] = item;
                return;
            }

            if (capacity <= items.len) {
                usize old_capacity = capacity;
                capacity *= 2;
                Slice<T> buff = alloc.realloc(items.ptr.slice(0, old_capacity), capacity);
                items = buff.slice(0, items.len + 1);
                items[items.len - 1] = item;
                return;
            }

            items = items.ptr.slice(0, items.len + 1);
            items[items.len - 1] = item;
            return;
        }

        T pop(heap::Allocator alloc) {
            if (items.len < capacity/4) {
                usize new_capacity = capacity/2;
                Slice<T> buff = alloc.realloc(items.ptr.slice(0, capacity), new_capacity);
                items = buff.slice(0, new_capacity);
            }

            T result = items[items.len - 1];
            items = items.slice(0, items.len - 1);
            return result;
        }
    };


    // TODO: made it a macro/a generic
    void println(const char* str) {
        std::printf("%s\n", str);
    }
    
    template <typename T, typename... Args>
    void print(const T& first, const Args&... args) {
        std::printf(first, args...);
    }
}
