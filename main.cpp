#include"stj/stj.hpp"

void foo(Slice<i32> items) {
    usize sum = 0;
    for(size_t i = 0; i < items.len; i++) {
        sum += items[i];
    }
    std::cout << "sum: " << sum << std::endl;
}

int main() {
    auto malloc = stj::heap::c_allocator;
    
    auto slice = malloc.alloc<i32>(10);
    defer (free(slice.ptr.raw_ptr));

    for(size_t i = 0; i < slice.len; i++) {
        slice[i] = i;
    }
    
    foo(slice.slice(5));

    auto list = stj::ArrayList<i32>::init();
    defer (list.deinit(malloc));

    list.append(malloc, 12);
    list.append(malloc, 11);
    list.append(malloc, 10);
    list.append(malloc, 69);
    list.append(malloc, 69);
    
    for(size_t i = 0; i < 10000; i++) {
        list.append(malloc, i);
    }
    
    foo(list.items);
}
