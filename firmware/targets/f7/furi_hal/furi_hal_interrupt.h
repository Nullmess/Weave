#pragma once

#include <stm32wbxx_ll_tim.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Timer ISR */
typedef void (*FuriHalInterruptISR)(void* context);

typedef enum {
    // TIM1, TIM16, TIM17
    FuriHalInterruptIdTim1TrgComTim17,
    FuriHalInterruptIdTim1Cc,
    FuriHalInterruptIdTim1UpTim16,

    // TIM2
    FuriHalInterruptIdTIM2,

    // DMA1
    FuriHalInterruptIdDma1Ch1,
    FuriHalInterruptIdDma1Ch2,
    FuriHalInterruptIdDma1Ch3,
    FuriHalInterruptIdDma1Ch4,
    FuriHalInterruptIdDma1Ch5,
    FuriHalInterruptIdDma1Ch6,
    FuriHalInterruptIdDma1Ch7,

    // DMA2
    FuriHalInterruptIdDma2Ch1,
    FuriHalInterruptIdDma2Ch2,
    FuriHalInterruptIdDma2Ch3,
    FuriHalInterruptIdDma2Ch4,
    FuriHalInterruptIdDma2Ch5,
    FuriHalInterruptIdDma2Ch6,
    FuriHalInterruptIdDma2Ch7,

    // RCC
    FuriHalInterruptIdRcc,

    // Comp
    FuriHalInterruptIdCOMP,

    // RTC
    FuriHalInterruptIdRtcAlarm,

    // HSEM
    FuriHalInterruptIdHsem,

    // LPTIMx
    FuriHalInterruptIdLpTim1,
    FuriHalInterruptIdLpTim2,

    //UARTx
    FuriHalInterruptIdUart1,

    //LPUARTx
    FuriHalInterruptIdLpUart1,

    // Service value
    FuriHalInterruptIdMax,
} FuriHalInterruptId;

typedef enum {
    FuriHalInterruptPriorityLowest =
        -3, /* Priorité minimale ; OS primitives ISR-safe admises */
    FuriHalInterruptPriorityLower =
        -2, /* Priorité très basse ; OS primitives ISR-safe admises */
    FuriHalInterruptPriorityLow =
        -1, /* Priorité basse ; OS primitives ISR-safe admises */
    FuriHalInterruptPriorityNormal =
        0, /* Priorité normale par défaut ; OS primitives ISR-safe admises */
    FuriHalInterruptPriorityHigh =
        1, /* Priorité haute ; OS primitives ISR-safe admises */
    FuriHalInterruptPriorityHigher =
        2, /* Priorité très haute ; OS primitives ISR-safe admises */
    FuriHalInterruptPriorityHighest =
        3, /* Priorité maximale ordinaire ; OS primitives ISR-safe admises */

    /* Special group, read docs first(ALL OF THEM: especially FreeRTOS configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY) */
    FuriHalInterruptPriorityKamiSama =
        6, /* Priorité critique : aucune primitive OS ; interaction hardware LL uniquement */
} FuriHalInterruptPriority;

void furi_hal_interrupt_init(void);

/* Flags IRQ non effacés automatiquement ; ISR NULL désassigne */

void furi_hal_interrupt_set_isr(FuriHalInterruptId index, FuriHalInterruptISR isr, void* context);

/* Flags IRQ non effacés automatiquement ; ISR NULL désassigne */

void furi_hal_interrupt_set_isr_ex(
    FuriHalInterruptId index,
    FuriHalInterruptPriority priority,
    FuriHalInterruptISR isr,
    void* context);

/* Exception depuis IPSR ; NULL si nom IRQ absent */

const char* furi_hal_interrupt_get_name(uint8_t exception_number);

/* Temps ISR cumulé en clocks CPU */

uint32_t furi_hal_interrupt_get_time_in_isr_total(void);

#ifdef __cplusplus
}
#endif
