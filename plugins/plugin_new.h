// plugins/plugin_new.h
// Minimal placement new declaration for plugin builds (no libstdc++).
#pragma once

#include <stddef.h>

// Placement new operators
inline void* operator new(size_t, void* ptr) noexcept { return ptr; }
inline void* operator new[](size_t, void* ptr) noexcept { return ptr; }
