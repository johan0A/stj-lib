#include"stj.hpp"

void foo(Slice<i32> items) {
    usize sum = 0;
    for(usize i = 0; i < items.len; i++) {
        sum += items[i];
    }
}

enum MathErr {
    OverFlow,
    UnderFlow,
};

Result<i64, MathErr> add(i32 a, i32 b) {
    if (b > 0 && a > 1000 - b) return MathErr::OverFlow;
    if (b < 0 && a < -1000 - b) return MathErr::UnderFlow;
    return a + b;
}

enum Err {
    FOO,
    BAR,
};

Result<i32, Err, MathErr> bar() {
    auto allocator = stj::heap::c_allocator;
    
    auto slice = allocator.alloc<i32>(10);
    defer (allocator.free(slice));

    for(usize i = 0; i < slice.len; i++) {
        slice[i] = i;
    }
    
    foo(slice.slice(5));

    auto list = stj::ArrayList<i32>::init();
    defer (list.deinit(allocator));

    list.append(allocator, 12);
    list.append(allocator, 11);
    list.append(allocator, 20);
    list.append(allocator, 20);
    list.append(allocator, 20);
    
    list.pop(allocator);
    list.pop(allocator);

    for(usize i = 0; i < 10000; i++) {
        list.append(allocator, i);
    }

    foo(list.items);

    errdefer (stj::println("out!"));
    
    stj::print("sum: %d\n", TRY(add(10, 12)));
    stj::print("sum: %d\n", TRY(add(INT32_MAX , 12)));

    foo(list.items);

    return 12;
}

int main() {
    (void) bar();
}