#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* OS helpers : configurer et démarrer tick timer */

void furi_hal_os_init(void);

/* Advance OS tick counter
 */
void furi_hal_os_tick(void);

#ifdef __cplusplus
}
#endif
