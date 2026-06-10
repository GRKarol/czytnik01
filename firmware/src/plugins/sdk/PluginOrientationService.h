// firmware/src/plugins/sdk/PluginOrientationService.h
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLUGIN_ORIENTATION_LANDSCAPE = 0,
    PLUGIN_ORIENTATION_PORTRAIT_A = 1,
    PLUGIN_ORIENTATION_PORTRAIT_B = 2,
    PLUGIN_ORIENTATION_LANDSCAPE_FLIPPED = 3,
} PluginOrientation;

typedef struct PluginOrientationService {
    PluginOrientation (*currentOrientation)(void);
    void (*setUiOrientation)(PluginOrientation orientation);
} PluginOrientationService;

#ifdef __cplusplus
}
#endif
