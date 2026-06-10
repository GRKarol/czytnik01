// firmware/src/plugins/sdk/PluginImuService.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginImuService {
    bool (*readAccelerometer)(float* x, float* y, float* z);
    bool (*available)(void);
} PluginImuService;

#ifdef __cplusplus
}
#endif
