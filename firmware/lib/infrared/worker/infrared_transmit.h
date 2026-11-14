#pragma once

#include <furi_hal_infrared.h>
#include <infrared.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* times : nombre transmissions */

void infrared_send(const InfraredMessage* message, int times);

/* start_from_mark : true Mark, false Space */

void infrared_send_raw(const uint32_t timings[], uint32_t timings_cnt, bool start_from_mark);

/* start_from_mark contrôle niveau initial ; duty_cycle/frequency PWM */

void infrared_send_raw_ext(
    const uint32_t timings[],
    uint32_t timings_cnt,
    bool start_from_mark,
    uint32_t frequency,
    float duty_cycle);

#ifdef __cplusplus
}
#endif
