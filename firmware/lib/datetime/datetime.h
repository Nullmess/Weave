#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Time
    uint8_t hour; /* Heure au format 24 h : 0-23 */
    uint8_t minute; /**< Minute: 0-59 */
    uint8_t second; /**< Second: 0-59 */
    // Date
    uint8_t day; /**< Current day: 1-31 */
    uint8_t month; /**< Current month: 1-12 */
    uint16_t year; /**< Current year: 2000-2099 */
    uint8_t weekday; /**< Current weekday: 1-7 */
} DateTime;

bool datetime_validate_datetime(DateTime* datetime);

/* DateTime UTC ; secondes depuis UNIX epoch */

uint32_t datetime_datetime_to_timestamp(DateTime* datetime);

/* Timestamp UNIX secondes vers DateTime UTC */

void datetime_timestamp_to_datetime(uint32_t timestamp, DateTime* datetime);

/* Calendrier grégorien */

uint16_t datetime_get_days_per_year(uint16_t year);

/* Année bissextile grégorienne */

bool datetime_is_leap_year(uint16_t year);

/* Mois 1=janvier ; leap_year adapte février */

uint8_t datetime_get_days_per_month(bool leap_year, uint8_t month);

#ifdef __cplusplus
}
#endif
