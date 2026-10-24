#include <stdatomic.h>

atomic_int x = 0;

void add(void) {
    atomic_fetch_add(&x, 1);
}

void exchange(void) {
    atomic_exchange(&x, 42);
}

int cas(void) {
    int expected = 42;
    return atomic_compare_exchange_strong(&x, &expected, 100);
}
