
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <furi_hal_resources.h>

#ifdef __cplusplus
extern "C" {
#endif

void furi_hal_vibro_init(void);

void furi_hal_vibro_on(bool value);

#ifdef __cplusplus
}
#endif
