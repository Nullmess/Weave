/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cs_time.h"

#if CS_ENABLE_STDIO

#ifndef _WIN32
#include <stddef.h>
/* ARMCC sans sys/time.h */

#if !(defined(__ARMCC_VERSION) || defined(__ICCARM__)) && !defined(__TI_COMPILER_VERSION__) && \
    (!defined(CS_PLATFORM) || CS_PLATFORM != CS_P_NXP_LPC)
#include <sys/time.h>
#endif
#else
#include <windows.h>
#endif

double cs_time(void) WEAK;
double cs_time(void) {
    double now;
#ifndef _WIN32
    struct timeval tv;
    if(gettimeofday(&tv, NULL  ) != 0) return 0;
    now = (double)tv.tv_sec + (((double)tv.tv_usec) / (double)1000000.0);
#else
    SYSTEMTIME sysnow;
    FILETIME ftime;
    GetLocalTime(&sysnow);
    SystemTimeToFileTime(&sysnow, &ftime);
    /* VC6 : conversion int64→double au lieu de uint64 */
    /* FILETIME : unités 100 ns depuis 1601 */
    /* Conversion Unix : secondes moins 11644473600 */

    now = (double)(((int64_t)ftime.dwLowDateTime + ((int64_t)ftime.dwHighDateTime << 32)) /
                   10000000.0) -
          11644473600;
#endif
    return now;
}

double cs_timegm(const struct tm* tm) {
    /* Offsets mensuels hors années bissextiles */
    static const int month_day[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    int month = tm->tm_mon % 12;
    int year = tm->tm_year + tm->tm_mon / 12;
    int year_for_leap;
    int64_t rt;

    if(month < 0) { /* Modulo négatif reste négatif */
        month += 12;
        --year;
    }

    /* Nombre de févriers depuis 1900 */
    year_for_leap = (month > 1) ? year + 1 : year;

    rt = tm->tm_sec
         + 60 * (tm->tm_min
                 + 60 * (tm->tm_hour
                         + 24 * (month_day[month] + tm->tm_mday - 1
                                 + 365 * (year - 70)
                                 + (year_for_leap - 69) / 4 /* Bissextile tous les quatre ans */
                                 - (year_for_leap - 1) / 100 /* Sauf siècles */
                                 + (year_for_leap + 299) / 400))); /* Sauf multiples de 400 */
    return rt < 0 ? -1 : (double)rt;
}

#endif