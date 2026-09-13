// firmware/src/plugins/sdk/SettingsStore.cpp
#include "SettingsStore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace settingsstore {

namespace {

// Plugin settings files are a handful of short lines (a preset index, a
// couple of tracking/anchor numbers) — these caps are generous for that
// and keep everything on the stack, no heap.
constexpr uint32_t kMaxFileBytes = 512;
constexpr size_t kMaxLines = 16;
constexpr size_t kMaxKeyEqLen = 48;

// Splits nul-terminated `buf` in place into up to kMaxLines lines (strips
// '\n' and a trailing '\r' from each), returning how many were found.
size_t splitLines(char* buf, char* lines[], size_t maxLines) {
    size_t count = 0;
    char* cursor = buf;
    while (cursor && *cursor && count < maxLines) {
        char* newline = strchr(cursor, '\n');
        if (newline) *newline = '\0';
        size_t len = strlen(cursor);
        if (len > 0 && cursor[len - 1] == '\r') cursor[len - 1] = '\0';
        lines[count++] = cursor;
        cursor = newline ? newline + 1 : nullptr;
    }
    return count;
}

}  // namespace

bool loadInt(ReadFileFn readFile, const char* relativePath, const char* key, int32_t& out,
             int32_t min, int32_t max, int32_t def) {
    out = def;
    if (!readFile || !relativePath || !key) return false;

    char buf[kMaxFileBytes];
    int bytesRead = readFile(relativePath, reinterpret_cast<uint8_t*>(buf), sizeof(buf) - 1);
    if (bytesRead <= 0) return false;
    buf[bytesRead] = '\0';

    char keyEq[kMaxKeyEqLen];
    snprintf(keyEq, sizeof(keyEq), "%s=", key);
    size_t keyEqLen = strlen(keyEq);

    char* lines[kMaxLines];
    size_t lineCount = splitLines(buf, lines, kMaxLines);
    for (size_t i = 0; i < lineCount; ++i) {
        if (strncmp(lines[i], keyEq, keyEqLen) != 0) continue;
        long value = strtol(lines[i] + keyEqLen, nullptr, 10);
        if (value < min || value > max) return false;  // out stays at def
        out = static_cast<int32_t>(value);
        return true;
    }
    return false;
}

bool saveInt(ReadFileFn readFile, WriteFileFn writeFile, const char* relativePath, const char* key,
             int32_t value) {
    if (!writeFile || !relativePath || !key) return false;

    char buf[kMaxFileBytes] = {};
    int bytesRead = readFile ? readFile(relativePath, reinterpret_cast<uint8_t*>(buf), sizeof(buf) - 1) : -1;
    if (bytesRead < 0) bytesRead = 0;
    buf[bytesRead] = '\0';

    char keyEq[kMaxKeyEqLen];
    snprintf(keyEq, sizeof(keyEq), "%s=", key);
    size_t keyEqLen = strlen(keyEq);

    char* lines[kMaxLines];
    size_t lineCount = splitLines(buf, lines, kMaxLines);

    char valueLine[kMaxKeyEqLen];
    snprintf(valueLine, sizeof(valueLine), "%s=%ld", key, static_cast<long>(value));

    char out[kMaxFileBytes];
    size_t outLen = 0;
    bool replaced = false;
    for (size_t i = 0; i < lineCount; ++i) {
        const char* toWrite = lines[i];
        if (strncmp(lines[i], keyEq, keyEqLen) == 0) {
            toWrite = valueLine;
            replaced = true;
        }
        if (toWrite[0] == '\0') continue;  // drop blank lines
        int n = snprintf(out + outLen, sizeof(out) - outLen, "%s\n", toWrite);
        if (n < 0 || outLen + static_cast<size_t>(n) >= sizeof(out)) return false;
        outLen += static_cast<size_t>(n);
    }
    if (!replaced) {
        int n = snprintf(out + outLen, sizeof(out) - outLen, "%s\n", valueLine);
        if (n < 0 || outLen + static_cast<size_t>(n) >= sizeof(out)) return false;
        outLen += static_cast<size_t>(n);
    }

    return writeFile(relativePath, reinterpret_cast<const uint8_t*>(out), static_cast<uint32_t>(outLen));
}

}  // namespace settingsstore
