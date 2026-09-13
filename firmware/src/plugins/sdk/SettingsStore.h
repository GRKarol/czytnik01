// firmware/src/plugins/sdk/SettingsStore.h
#pragma once

#include <stdint.h>

namespace settingsstore {

// Same shape as PluginStorageService::readFile/writeFile — kept as plain
// function pointers (not the struct itself) so this module has zero
// dependency on the plugin SDK or SD_MMC and can be unit-tested/reused
// standalone.
using ReadFileFn = int (*)(const char* relativePath, uint8_t* buffer, uint32_t maxLen);
using WriteFileFn = bool (*)(const char* relativePath, const uint8_t* data, uint32_t len);

// Reads `key` from a `key=value`-per-line text file at relativePath. A
// value outside [min, max], a missing file, or a missing key all fall back
// to `def` (returned via `out`) — a hand-edited or corrupted file on the SD
// card can never hand a plugin a nonsensical number. Returns true only if
// `key` was found and its value was in range.
bool loadInt(ReadFileFn readFile, const char* relativePath, const char* key, int32_t& out,
             int32_t min, int32_t max, int32_t def);

// Writes `key=value` into relativePath, preserving any other keys already
// stored there (read-modify-write over the whole file — these are a
// handful of lines per plugin, not worth a real key-value store).
bool saveInt(ReadFileFn readFile, WriteFileFn writeFile, const char* relativePath, const char* key,
             int32_t value);

}  // namespace settingsstore
