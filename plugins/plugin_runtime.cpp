// plugins/plugin_runtime.cpp
// Minimal C++ runtime stubs for plugin binaries built with -nostdlib.
// Plugins use placement new (no heap allocation), so only linker-required
// stubs are provided here. Placement new is declared in plugin_new.h.

#include <stddef.h>
#include <stdint.h>

// Regular new/delete stubs — should never be called since plugins use
// placement new. Provide them to satisfy the linker if any implicit
// references exist.
void* operator new(size_t size) noexcept {
    (void)size;
    return nullptr;  // Will never be called
}

void* operator new[](size_t size) noexcept {
    (void)size;
    return nullptr;
}

void operator delete(void* ptr) noexcept { (void)ptr; }
void operator delete[](void* ptr) noexcept { (void)ptr; }
void operator delete(void* ptr, size_t) noexcept { (void)ptr; }
void operator delete[](void* ptr, size_t) noexcept { (void)ptr; }

// Pure virtual function handler — called if a pure virtual is invoked
extern "C" void __cxa_pure_virtual() {
    while (1) {}  // Hang — should never happen
}

// __cxa_atexit stub — static destructors not needed in plugin context
extern "C" int __cxa_atexit(void (*)(void*), void*, void*) {
    return 0;
}

// framework-arduinoespressif32's own main.cpp (loopTask) always calls
// setup()/loop() — even though plugin binaries never actually boot through
// the normal Arduino entry point. Only the .plugin_header/.plugin_vtable
// sections of this ELF are ever extracted and used (see
// tools/pio_plugin_build.py); the rest, including this "sketch", is inert.
// These stubs exist purely to satisfy the linker.
void setup() {}
void loop() {}
