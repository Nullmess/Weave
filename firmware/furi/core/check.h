/* Crash : message via r12, r0-r10 sauvegardés/restaurés via RAM2 */
/* Seul r11 est perdu */
#pragma once

#include <m-core.h>
#include "common_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Flags plutôt que pointers : environ 4 bytes économisés */
#define __FURI_ASSERT_MESSAGE_FLAG (0x01)
#define __FURI_CHECK_MESSAGE_FLAG  (0x02)

FURI_NORETURN void __furi_crash_implementation(void);

FURI_NORETURN void __furi_halt_implementation(void);

/* Message affiché après reboot */
#define __furi_crash(message)                                 \
    do {                                                      \
        register const void* r12 asm("r12") = (void*)message; \
        asm volatile("sukima%=:" : : "r"(r12));               \
        __furi_crash_implementation();                        \
    } while(0)

/* Message const char* optionnel */
#define furi_crash(...) M_APPLY(__furi_crash, M_IF_EMPTY(__VA_ARGS__)((NULL), (__VA_ARGS__)))

/* Halt avec message */
#define __furi_halt(message)                                  \
    do {                                                      \
        register const void* r12 asm("r12") = (void*)message; \
        asm volatile("sukima%=:" : : "r"(r12));               \
        __furi_halt_implementation();                         \
    } while(0)

/* Message const char* optionnel */
#define furi_halt(...) M_APPLY(__furi_halt, M_IF_EMPTY(__VA_ARGS__)((NULL), (__VA_ARGS__)))

#define __furi_check(__e, __m) \
    do {                       \
        if(!(__e)) {           \
            __furi_crash(__m); \
        }                      \
    } while(0)

/* Message const char* optionnel */
#define furi_check(...) \
    M_APPLY(__furi_check, M_DEFAULT_ARGS(2, (__FURI_CHECK_MESSAGE_FLAG), __VA_ARGS__))

/* Vérification uniquement en build debug */
#ifdef FURI_DEBUG
#define __furi_assert(__e, __m) \
    do {                        \
        if(!(__e)) {            \
            __furi_crash(__m);  \
        }                       \
    } while(0)
#else
#define __furi_assert(__e, __m) \
    do {                        \
        ((void)(__e));          \
        ((void)(__m));          \
    } while(0)
#endif

/* Vérification uniquement en build debug ; message optionnel */
#define furi_assert(...) \
    M_APPLY(__furi_assert, M_DEFAULT_ARGS(2, (__FURI_ASSERT_MESSAGE_FLAG), __VA_ARGS__))

#define furi_break(__e)             \
    do {                            \
        if(!(__e)) {                \
            asm volatile("bkpt 0"); \
        }                           \
    } while(0)

#ifdef __cplusplus
}
#endif
