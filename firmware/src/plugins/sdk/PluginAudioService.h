// firmware/src/plugins/sdk/PluginAudioService.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginAudioService {
    bool (*beep)(void);
    bool (*available)(void);
} PluginAudioService;

#ifdef __cplusplus
}
#endif
