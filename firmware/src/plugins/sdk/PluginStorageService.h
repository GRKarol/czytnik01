// firmware/src/plugins/sdk/PluginStorageService.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginStorageService {
    // Sandboxed file operations — paths relative to /plugins/{id}/
    bool (*fileExists)(const char* relativePath);
    int (*readFile)(const char* relativePath, uint8_t* buffer, uint32_t maxLen);
    bool (*writeFile)(const char* relativePath, const uint8_t* data, uint32_t len);
    bool (*deleteFile)(const char* relativePath);
    bool (*mkdir)(const char* relativePath);
    bool (*renameFile)(const char* fromRelativePath, const char* toRelativePath);

    /// Bounded int read: parses a `key=value` line out of a small text file
    /// at relativePath (same sandboxing as readFile). A value outside
    /// [min, max], or a missing file/key, falls back to `def` (written to
    /// `out`) — a hand-edited or corrupted file can never hand the plugin a
    /// nonsensical number. Returns true only if `key` was found in range.
    /// See firmware/src/plugins/sdk/SettingsStore.h for the shared parser.
    bool (*loadInt)(const char* relativePath, const char* key, int32_t* out, int32_t min,
                    int32_t max, int32_t def);
    /// Bounded int write: read-modify-write of the same `key=value` file,
    /// preserving any other keys already stored there.
    bool (*saveInt)(const char* relativePath, const char* key, int32_t value);
} PluginStorageService;

#ifdef __cplusplus
}
#endif
