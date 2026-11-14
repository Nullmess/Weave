
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <core/string.h>
#include <toolbox/property.h>

#ifdef __cplusplus
extern "C" {
#endif

void furi_hal_info_get_api_version(uint16_t* major, uint16_t* minor);

void furi_hal_info_get(PropertyValueCallback callback, char sep, void* context);

#ifdef __cplusplus
}
#endif
