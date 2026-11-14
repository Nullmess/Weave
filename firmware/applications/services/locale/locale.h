#pragma once

#include <stdbool.h>
#include <furi.h>
#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LocaleMeasurementUnitsMetric = 0, /**< Metric measurement units */
    LocaleMeasurementUnitsImperial = 1, /**< Imperial measurement units */
} LocaleMeasurementUnits;

typedef enum {
    LocaleTimeFormat24h = 0, /**< 24-hour format */
    LocaleTimeFormat12h = 1, /**< 12-hour format */
} LocaleTimeFormat;

typedef enum {
    LocaleDateFormatDMY = 0, /**< Day/Month/Year */
    LocaleDateFormatMDY = 1, /**< Month/Day/Year */
    LocaleDateFormatYMD = 2, /**< Year/Month/Day */
} LocaleDateFormat;

LocaleMeasurementUnits locale_get_measurement_unit(void);

void locale_set_measurement_unit(LocaleMeasurementUnits format);

float locale_fahrenheit_to_celsius(float temp_f);

float locale_celsius_to_fahrenheit(float temp_c);

LocaleTimeFormat locale_get_time_format(void);

void locale_set_time_format(LocaleTimeFormat format);

void locale_format_time(
    FuriString* out_str,
    const DateTime* datetime,
    const LocaleTimeFormat format,
    const bool show_seconds);

LocaleDateFormat locale_get_date_format(void);

void locale_set_date_format(LocaleDateFormat format);

void locale_format_date(
    FuriString* out_str,
    const DateTime* datetime,
    const LocaleDateFormat format,
    const char* separator);

#ifdef __cplusplus
}
#endif
