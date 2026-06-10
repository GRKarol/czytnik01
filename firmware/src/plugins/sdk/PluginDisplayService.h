// firmware/src/plugins/sdk/PluginDisplayService.h
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginDisplayService {
    void (*renderFocusTimerScreen)(const char* mode, const char* genre,
                                   const char* timer, const char* instruction,
                                   const char* footer, int progressPercent,
                                   bool breakAccent);
    void (*renderStatus)(const char* title, const char* line1, const char* line2);
    void (*renderProgress)(const char* title, const char* line1,
                           const char* line2, int progressPercent);
    void (*renderMenu)(const char* const* items, uint8_t itemCount,
                       uint8_t selectedIndex);
    void (*renderCenteredWord)(const char* word);
    void (*setDarkMode)(bool dark);
    int (*logicalWidth)(void);
    int (*logicalHeight)(void);
} PluginDisplayService;

#ifdef __cplusplus
}
#endif
