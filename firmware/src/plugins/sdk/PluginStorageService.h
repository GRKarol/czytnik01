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
} PluginStorageService;

#ifdef __cplusplus
}
#endif
