#include <furi_hal_mpu.h>
#include <stm32wbxx_ll_cortex.h>

#include <stm32wb55_linker.h>
#include <core/check.h>

#define FURI_HAL_MPU_ATTRIBUTES                                                     \
    (LL_MPU_ACCESS_BUFFERABLE | LL_MPU_ACCESS_CACHEABLE | LL_MPU_ACCESS_SHAREABLE | \
     LL_MPU_TEX_LEVEL1 | LL_MPU_INSTRUCTION_ACCESS_ENABLE)

#define FURI_HAL_MPU_STACK_PROTECT_REGION FuriHalMPURegionSize32B

void furi_hal_mpu_init(void) {
    furi_hal_mpu_enable();

    /* Protection déréférencement NULL */
    furi_hal_mpu_protect_no_access(FuriHalMpuRegionNULL, 0x00, FuriHalMPURegionSize1MB);
    furi_hal_mpu_protect_no_access(
        FuriHalMpuRegionMainStack,
        (uint32_t)((uintptr_t)&_stack_end - (uintptr_t)&_stack_size),
        FuriHalMPURegionSize32B);
}

void furi_hal_mpu_enable(void) {
    LL_MPU_Enable(LL_MPU_CTRL_PRIVILEGED_DEFAULT);
    __DSB();
    __ISB();
}

void furi_hal_mpu_disable(void) {
    __DSB();
    LL_MPU_Disable();
    __DSB();
    __ISB();
}

/* Do not globally disable the MPU during a region update. Preserve the
 * caller's interrupt mask (also used from the RTOS switch hook). */
static void furi_hal_mpu_configure(
    FuriHalMpuRegion region, uint32_t address, FuriHalMPURegionSize size, uint32_t access) {
    furi_check((unsigned)region <= FuriHalMpuRegion7);
    furi_check((unsigned)size >= FuriHalMPURegionSize32B &&
               (unsigned)size <= FuriHalMPURegionSize4GB);
    const uint64_t bytes = UINT64_C(1) << ((unsigned)size + 1U);
    furi_check(((uint64_t)address & (bytes - 1U)) == 0U);
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    LL_MPU_DisableRegion(region);
    __DSB();
    __ISB();
    LL_MPU_ConfigRegion(region, 0x00, address,
        FURI_HAL_MPU_ATTRIBUTES | access | ((uint32_t)size << MPU_RASR_SIZE_Pos));
    __DSB();
    __ISB();
    __set_PRIMASK(primask);
}

void furi_hal_mpu_protect_no_access(
    FuriHalMpuRegion region, uint32_t address, FuriHalMPURegionSize size) {
    furi_hal_mpu_configure(region, address, size, LL_MPU_REGION_NO_ACCESS);
}

void furi_hal_mpu_protect_read_only(
    FuriHalMpuRegion region, uint32_t address, FuriHalMPURegionSize size) {
    furi_hal_mpu_configure(region, address, size, LL_MPU_REGION_PRIV_RO_URO);
}

void furi_hal_mpu_protect_disable(FuriHalMpuRegion region) {
    furi_check((unsigned)region <= FuriHalMpuRegion7);
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    LL_MPU_DisableRegion(region);
    __DSB();
    __ISB();
    __set_PRIMASK(primask);
}

void furi_hal_mpu_set_stack_protection(uint32_t* stack) {
    /* Adresse protection alignée sur taille région requise */
    uint32_t stack_ptr = (uint32_t)(uintptr_t)stack;
    uint32_t mask = ((1U << (FURI_HAL_MPU_STACK_PROTECT_REGION + 1U)) - 1U);
    stack_ptr &= ~mask;
    if(stack_ptr < (uint32_t)(uintptr_t)stack) {
        furi_check(stack_ptr <= UINT32_MAX - (mask + 1U));
        stack_ptr += mask + 1U;
    }

    furi_hal_mpu_protect_read_only(
        FuriHalMpuRegionThreadStack, stack_ptr, FURI_HAL_MPU_STACK_PROTECT_REGION);
}
