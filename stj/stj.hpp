#pragma once

#include"base.hpp"

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
                // Slice<u8> bytes = vtable.alloc(impl_data, byte_size);
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
}
