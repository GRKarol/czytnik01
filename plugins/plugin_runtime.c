/* plugins/plugin_runtime.c — Minimal C runtime for plugin binaries.
 * Provides memset/memcpy/memmove which the compiler may emit implicitly.
 * Linked with -nostdlib plugins that cannot use libc. */

#include <stddef.h>
#include <stdint.h>

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s2 = (const unsigned char *)src;
    while (n--) *d++ = *s2++;
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s2 = (const unsigned char *)src;
    if (d < s2) {
        while (n--) *d++ = *s2++;
    } else {
        d += n;
        s2 += n;
        while (n--) *--d = *--s2;
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    while (n--) {
        if (*a != *b) return *a - *b;
        a++;
        b++;
    }
    return 0;
}

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

/* __cxa_pure_virtual — called if a pure virtual is somehow invoked */
void __cxa_pure_virtual(void) {
    while (1) {}
}
