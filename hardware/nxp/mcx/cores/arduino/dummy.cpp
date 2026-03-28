// Dummy source to satisfy Arduino IDE core.a requirement
// All real code is in the prebuilt library

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

// C++ operators
void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void* ptr) noexcept {
    free(ptr);
}

void operator delete[](void* ptr) noexcept {
    free(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    free(ptr);
}

void* __dso_handle = (void*)0;

// Heap management
extern char _pvHeapStart;  // defined in linker script
extern char _pvHeapLimit;  // defined in linker script
static char* heap_end = 0;

// Newlib syscall stubs
extern "C" {
    void _init(void) {}
    void _fini(void) {}

    __attribute__((noreturn))
    void _exit(int) { while(1) {} }

    int _kill(int, int) { return -1; }
    int _getpid(void) { return 1; }

    void* _sbrk(intptr_t incr) {
        if (heap_end == 0) {
            heap_end = &_pvHeapStart;
        }
        char* prev_heap_end = heap_end;
        if (heap_end + incr > &_pvHeapLimit) {
            return (void*)-1;  // heap overflow
        }
        heap_end += incr;
        return (void*)prev_heap_end;
    }

    int _close(int) { return -1; }
    int _lseek(int, int, int) { return -1; }
    int _read(int, void*, int) { return -1; }
    int _write(int, const void*, int) { return -1; }
    int _fstat(int, void*) { return -1; }
    int _isatty(int) { return 1; }
}
