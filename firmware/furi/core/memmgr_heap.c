/*
 * FreeRTOS Kernel V11.1.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of https://www.FreeRTOS.org for more information.
 */

#include "memmgr_heap.h"
#include "check.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stm32wbxx.h>
#include <stm32wb55_linker.h>
#include <core/log.h>
#include <core/common_defines.h>

// -V::562
// -V::650

/* MPU_WRAPPERS_INCLUDED_FROM_API_FILE empêche les wrappers task.h */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include <FreeRTOS.h>
#include <task.h>

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if(configSUPPORT_DYNAMIC_ALLOCATION == 0)
#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

#ifndef configHEAP_CLEAR_MEMORY_ON_FREE
#define configHEAP_CLEAR_MEMORY_ON_FREE 0
#endif

/* Taille minimale des blocs requise */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/* Hypothèse : bytes de 8 bits */
#define heapBITS_PER_BYTE ((size_t)8)

#define heapSIZE_MAX (~((size_t)0))

#define heapMULTIPLY_WILL_OVERFLOW(a, b) (((a) > 0) && ((b) > (heapSIZE_MAX / (a))))

#define heapADD_WILL_OVERFLOW(a, b) ((a) > (heapSIZE_MAX - (b)))

#define heapSUBTRACT_WILL_UNDERFLOW(a, b) ((a) < (b))

/* MSB de xBlockSize : 1 application, 0 heap libre */
#define heapBLOCK_ALLOCATED_BITMASK         (((size_t)1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1))
#define heapBLOCK_SIZE_IS_VALID(xBlockSize) (((xBlockSize) & heapBLOCK_ALLOCATED_BITMASK) == 0)
#define heapBLOCK_IS_ALLOCATED(pxBlock) \
    (((pxBlock->xBlockSize) & heapBLOCK_ALLOCATED_BITMASK) != 0)
#define heapALLOCATE_BLOCK(pxBlock) ((pxBlock->xBlockSize) |= heapBLOCK_ALLOCATED_BITMASK)
#define heapFREE_BLOCK(pxBlock)     ((pxBlock->xBlockSize) &= ~heapBLOCK_ALLOCATED_BITMASK)

/* Bornes heap fournies par le linker */
uint8_t* ucHeap = (uint8_t*)&__heap_start__;

/* Blocs libres triés par adresse mémoire */
typedef struct A_BLOCK_LINK {
    struct A_BLOCK_LINK* pxNextFreeBlock;
    size_t xBlockSize;
} BlockLink_t;

/* configENABLE_HEAP_PROTECTOR=1 : protection pointers par canary */
#if(configENABLE_HEAP_PROTECTOR == 1)

/* Canary aléatoire fourni par l’application */
extern void vApplicationGetRandomHeapCanary(portPOINTER_SIZE_TYPE* pxHeapCanary);

/* Canary de protection des pointers heap */
PRIVILEGED_DATA static portPOINTER_SIZE_TYPE xHeapCanary;

/* XOR canary : corruption détectée par validation des pointers */
#define heapPROTECT_BLOCK_POINTER(pxBlock) \
    ((BlockLink_t*)(((portPOINTER_SIZE_TYPE)(pxBlock)) ^ xHeapCanary))
#else

#define heapPROTECT_BLOCK_POINTER(pxBlock) (pxBlock)

#endif /* configENABLE_HEAP_PROTECTOR */

/* Pointer de bloc limité aux bornes du heap */
#define heapVALIDATE_BLOCK_POINTER(pxBlock)      \
    configASSERT(                                \
        ((uint8_t*)(pxBlock) >= &(ucHeap[0])) && \
        ((uint8_t*)(pxBlock) <= &(ucHeap[configTOTAL_HEAP_SIZE - 1])))

/* Fusion des blocs libres adjacents */
static void prvInsertBlockIntoFreeList(BlockLink_t* pxBlockToInsert) PRIVILEGED_FUNCTION;

/* Initialisation automatique au premier pvPortMalloc */
static void prvHeapInit(void) PRIVILEGED_FUNCTION;

/* En-tête des blocs obligatoirement aligné */
static const size_t xHeapStructSize = (sizeof(BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) &
                                      ~((size_t)portBYTE_ALIGNMENT_MASK);

PRIVILEGED_DATA static BlockLink_t xStart;
PRIVILEGED_DATA static BlockLink_t* pxEnd = NULL;

/* Statistiques allocations/bytes libres ; fragmentation non mesurée */
PRIVILEGED_DATA static size_t xFreeBytesRemaining = (size_t)0U;
PRIVILEGED_DATA static size_t xMinimumEverFreeBytesRemaining = (size_t)0U;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulAllocations = (size_t)0U;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulFrees = (size_t)0U;

#include <m-dict.h>

DICT_DEF2(MemmgrHeapAllocDict, uint32_t, uint32_t) //-V1048

DICT_DEF2( //-V1048
    MemmgrHeapThreadDict,
    uint32_t,
    M_DEFAULT_OPLIST,
    MemmgrHeapAllocDict_t,
    DICT_OPLIST(MemmgrHeapAllocDict))

static MemmgrHeapThreadDict_t memmgr_heap_thread_dict = {0};
static volatile uint32_t memmgr_heap_thread_trace_depth = 0;

void memmgr_heap_init(void) {
    MemmgrHeapThreadDict_init(memmgr_heap_thread_dict);
}

void memmgr_heap_enable_thread_trace(FuriThreadId thread_id) {
    vTaskSuspendAll();
    {
        memmgr_heap_thread_trace_depth++;
        furi_check(MemmgrHeapThreadDict_get(memmgr_heap_thread_dict, (uint32_t)thread_id) == NULL);
        MemmgrHeapAllocDict_t alloc_dict;
        MemmgrHeapAllocDict_init(alloc_dict);
        MemmgrHeapThreadDict_set_at(memmgr_heap_thread_dict, (uint32_t)thread_id, alloc_dict);
        MemmgrHeapAllocDict_clear(alloc_dict);
        memmgr_heap_thread_trace_depth--;
    }
    (void)xTaskResumeAll();
}

void memmgr_heap_disable_thread_trace(FuriThreadId thread_id) {
    vTaskSuspendAll();
    {
        memmgr_heap_thread_trace_depth++;
        furi_check(MemmgrHeapThreadDict_erase(memmgr_heap_thread_dict, (uint32_t)thread_id));
        memmgr_heap_thread_trace_depth--;
    }
    (void)xTaskResumeAll();
}

size_t memmgr_heap_get_thread_memory(FuriThreadId thread_id) {
    size_t leftovers = MEMMGR_HEAP_UNKNOWN;
    vTaskSuspendAll();
    {
        memmgr_heap_thread_trace_depth++;
        MemmgrHeapAllocDict_t* alloc_dict =
            MemmgrHeapThreadDict_get(memmgr_heap_thread_dict, (uint32_t)thread_id);
        if(alloc_dict) {
            leftovers = 0;
            MemmgrHeapAllocDict_it_t alloc_dict_it;
            for(MemmgrHeapAllocDict_it(alloc_dict_it, *alloc_dict);
                !MemmgrHeapAllocDict_end_p(alloc_dict_it);
                MemmgrHeapAllocDict_next(alloc_dict_it)) {
                MemmgrHeapAllocDict_itref_t* data = MemmgrHeapAllocDict_ref(alloc_dict_it);
                if(data->key != 0) {
                    uint8_t* puc = (uint8_t*)data->key;
                    puc -= xHeapStructSize;
                    BlockLink_t* pxLink = (void*)puc;

                    if((pxLink->xBlockSize & heapBLOCK_ALLOCATED_BITMASK) &&
                       pxLink->pxNextFreeBlock == NULL) {
                        leftovers += data->value;
                    }
                }
            }
        }
        memmgr_heap_thread_trace_depth--;
    }
    (void)xTaskResumeAll();
    return leftovers;
}

#undef traceMALLOC
static inline void traceMALLOC(void* pointer, size_t size) {
    FuriThreadId thread_id = furi_thread_get_current_id();
    if(thread_id && memmgr_heap_thread_trace_depth == 0) {
        memmgr_heap_thread_trace_depth++;
        MemmgrHeapAllocDict_t* alloc_dict =
            MemmgrHeapThreadDict_get(memmgr_heap_thread_dict, (uint32_t)thread_id);
        if(alloc_dict) {
            MemmgrHeapAllocDict_set_at(*alloc_dict, (uint32_t)pointer, (uint32_t)size);
        }
        memmgr_heap_thread_trace_depth--;
    }
}

#undef traceFREE
static inline void traceFREE(void* pointer, size_t size) {
    UNUSED(size);
    FuriThreadId thread_id = furi_thread_get_current_id();
    if(thread_id && memmgr_heap_thread_trace_depth == 0) {
        memmgr_heap_thread_trace_depth++;
        MemmgrHeapAllocDict_t* alloc_dict =
            MemmgrHeapThreadDict_get(memmgr_heap_thread_dict, (uint32_t)thread_id);
        if(alloc_dict) {
            /* Un thread peut libérer une allocation d’un autre */
            const bool res = MemmgrHeapAllocDict_erase(*alloc_dict, (uint32_t)pointer);
            UNUSED(res);
        }
        memmgr_heap_thread_trace_depth--;
    }
}

size_t memmgr_heap_get_max_free_block(void) {
    HeapStats_t heap_stats;
    vPortGetHeapStats(&heap_stats);
    return heap_stats.xSizeOfLargestFreeBlockInBytes;
}

void memmgr_heap_printf_free_blocks(void) {
    BlockLink_t* pxBlock;
    /* Activer lorsque printf supportera le scheduler verrouillé */

    pxBlock = heapPROTECT_BLOCK_POINTER(xStart.pxNextFreeBlock);
    heapVALIDATE_BLOCK_POINTER(pxBlock);
    while(pxBlock->pxNextFreeBlock != heapPROTECT_BLOCK_POINTER(NULL)) {
        printf("A %p S %lu\r\n", (void*)pxBlock, (uint32_t)pxBlock->xBlockSize);
        pxBlock = heapPROTECT_BLOCK_POINTER(pxBlock->pxNextFreeBlock);
        heapVALIDATE_BLOCK_POINTER(pxBlock);
    }

}

void* pvPortMalloc(size_t xWantedSize) {
    BlockLink_t* pxBlock;
    BlockLink_t* pxPreviousBlock;
    BlockLink_t* pxNewBlockLink;
    void* pvReturn = NULL;
    size_t xToWipe = xWantedSize;
    size_t xAdditionalRequiredSize;
    size_t xAllocatedBlockSize = 0;

    if(FURI_IS_IRQ_MODE()) {
        furi_crash("memmgt in ISR");
    }

    if(xWantedSize > 0) {
        /* Taille demandée augmentée de l’en-tête BlockLink_t */
        if(heapADD_WILL_OVERFLOW(xWantedSize, xHeapStructSize) == 0) {
            xWantedSize += xHeapStructSize;

            /* Alignement obligatoire des blocs */
            if((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00) {
                /* Alignement en bytes requis */
                xAdditionalRequiredSize =
                    portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK);

                if(heapADD_WILL_OVERFLOW(xWantedSize, xAdditionalRequiredSize) == 0) {
                    xWantedSize += xAdditionalRequiredSize;
                } else {
                    xWantedSize = 0;
                }
            } else {
                mtCOVERAGE_TEST_MARKER();
            }
        } else {
            xWantedSize = 0;
        }
    } else {
        mtCOVERAGE_TEST_MARKER();
    }

    vTaskSuspendAll();
    {
        if(pxEnd == NULL) {
            prvHeapInit();
            memmgr_heap_init();
        } else {
            mtCOVERAGE_TEST_MARKER();
        }

        /* MSB de taille réservé à l’ownership, jamais à la taille */
        if(heapBLOCK_SIZE_IS_VALID(xWantedSize) != 0) {
            if((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining)) {
                pxPreviousBlock = &xStart;
                pxBlock = heapPROTECT_BLOCK_POINTER(xStart.pxNextFreeBlock);
                heapVALIDATE_BLOCK_POINTER(pxBlock);

                while((pxBlock->xBlockSize < xWantedSize) &&
                      (pxBlock->pxNextFreeBlock != heapPROTECT_BLOCK_POINTER(NULL))) {
                    pxPreviousBlock = pxBlock;
                    pxBlock = heapPROTECT_BLOCK_POINTER(pxBlock->pxNextFreeBlock);
                    heapVALIDATE_BLOCK_POINTER(pxBlock);
                }

                /* Aucun bloc suffisamment grand */
                if(pxBlock != pxEnd) {
                    /* Allocation après l’en-tête BlockLink_t */
                    pvReturn = (void*)(((uint8_t*)heapPROTECT_BLOCK_POINTER(
                                           pxPreviousBlock->pxNextFreeBlock)) +
                                       xHeapStructSize);
                    heapVALIDATE_BLOCK_POINTER(pvReturn);

                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    configASSERT(
                        heapSUBTRACT_WILL_UNDERFLOW(pxBlock->xBlockSize, xWantedSize) == 0);

                    if((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE) {
                        /* Cast void pour éviter warnings d’alignement */
                        pxNewBlockLink = (void*)(((uint8_t*)pxBlock) + xWantedSize);
                        configASSERT((((size_t)pxNewBlockLink) & portBYTE_ALIGNMENT_MASK) == 0);

                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        pxNewBlockLink->pxNextFreeBlock = pxPreviousBlock->pxNextFreeBlock;
                        pxPreviousBlock->pxNextFreeBlock =
                            heapPROTECT_BLOCK_POINTER(pxNewBlockLink);
                    } else {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if(xFreeBytesRemaining < xMinimumEverFreeBytesRemaining) {
                        xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
                    } else {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xAllocatedBlockSize = pxBlock->xBlockSize;

                    /* Bloc alloué : propriété application, aucun next libre */
                    heapALLOCATE_BLOCK(pxBlock);
                    pxBlock->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(NULL);
                    xNumberOfSuccessfulAllocations++;
                } else {
                    mtCOVERAGE_TEST_MARKER();
                }
            } else {
                mtCOVERAGE_TEST_MARKER();
            }
        } else {
            mtCOVERAGE_TEST_MARKER();
        }

        traceMALLOC(pvReturn, xAllocatedBlockSize);

        (void)xAllocatedBlockSize;
    }
    (void)xTaskResumeAll();

#if(configUSE_MALLOC_FAILED_HOOK == 1)
    {
        if(pvReturn == NULL) {
            vApplicationMallocFailedHook();
        } else {
            mtCOVERAGE_TEST_MARKER();
        }
    }
#endif

    configASSERT((((size_t)pvReturn) & (size_t)portBYTE_ALIGNMENT_MASK) == 0);

    furi_check(pvReturn, xWantedSize ? "out of memory" : "malloc(0)");
    pvReturn = memset(pvReturn, 0, xToWipe);
    return pvReturn;
}

void vPortFree(void* pv) {
    uint8_t* puc = (uint8_t*)pv;
    BlockLink_t* pxLink;

    if(FURI_IS_IRQ_MODE()) {
        furi_crash("memmgt in ISR");
    }

    if(pv != NULL) {
        /* BlockLink_t immédiatement avant la mémoire utilisateur */
        puc -= xHeapStructSize;

        pxLink = (void*)puc;

        heapVALIDATE_BLOCK_POINTER(pxLink);
        configASSERT(heapBLOCK_IS_ALLOCATED(pxLink) != 0);
        configASSERT(pxLink->pxNextFreeBlock == heapPROTECT_BLOCK_POINTER(NULL));

        if(heapBLOCK_IS_ALLOCATED(pxLink) != 0) {
            if(pxLink->pxNextFreeBlock == heapPROTECT_BLOCK_POINTER(NULL)) {
                heapFREE_BLOCK(pxLink);
#if(configHEAP_CLEAR_MEMORY_ON_FREE == 1)
                {
                    /* Détection underflow si xBlockSize corrompu */
                    if(heapSUBTRACT_WILL_UNDERFLOW(pxLink->xBlockSize, xHeapStructSize) == 0) {
                        (void)memset(
                            puc + xHeapStructSize, 0, pxLink->xBlockSize - xHeapStructSize);
                    }
                }
#endif

                vTaskSuspendAll();
                {
                    furi_assert((size_t)pv >= SRAM_BASE);
                    furi_assert((size_t)pv < SRAM_BASE + 1024 * 256);
                    furi_assert(pxLink->xBlockSize >= xHeapStructSize);
                    furi_assert((pxLink->xBlockSize - xHeapStructSize) < 1024 * 256);

                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE(pv, pxLink->xBlockSize);
                    prvInsertBlockIntoFreeList(((BlockLink_t*)pxLink));
                    xNumberOfSuccessfulFrees++;
                }
                (void)xTaskResumeAll();
            } else {
                mtCOVERAGE_TEST_MARKER();
            }
        } else {
            mtCOVERAGE_TEST_MARKER();
        }
    }
}

size_t xPortGetFreeHeapSize(void) {
    return xFreeBytesRemaining;
}

size_t xPortGetMinimumEverFreeHeapSize(void) {
    return xMinimumEverFreeBytesRemaining;
}

void xPortResetHeapMinimumEverFreeHeapSize(void) {
    xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
}

void vPortInitialiseBlocks(void) {

}

void* pvPortCalloc(size_t xNum, size_t xSize) {
    void* pv = NULL;

    if(heapMULTIPLY_WILL_OVERFLOW(xNum, xSize) == 0) {
        pv = pvPortMalloc(xNum * xSize);

        if(pv != NULL) {
            (void)memset(pv, 0, xNum * xSize);
        }
    }

    return pv;
}

static void prvHeapInit(void)
{
    BlockLink_t* pxFirstFreeBlock;
    portPOINTER_SIZE_TYPE uxStartAddress, uxEndAddress;
    size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

    /* Début du heap obligatoirement aligné */
    uxStartAddress = (portPOINTER_SIZE_TYPE)ucHeap;

    if((uxStartAddress & portBYTE_ALIGNMENT_MASK) != 0) {
        uxStartAddress += (portBYTE_ALIGNMENT - 1);
        uxStartAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
        xTotalHeapSize -= (size_t)(uxStartAddress - (portPOINTER_SIZE_TYPE)ucHeap);
    }

#if(configENABLE_HEAP_PROTECTOR == 1)
    { vApplicationGetRandomHeapCanary(&(xHeapCanary)); }
#endif

    /* xStart référence le premier bloc libre */
    xStart.pxNextFreeBlock = (void*)heapPROTECT_BLOCK_POINTER(uxStartAddress);
    xStart.xBlockSize = (size_t)0;

    /* pxEnd termine la liste et le heap */
    uxEndAddress = uxStartAddress + (portPOINTER_SIZE_TYPE)xTotalHeapSize;
    uxEndAddress -= (portPOINTER_SIZE_TYPE)xHeapStructSize;
    uxEndAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
    pxEnd = (BlockLink_t*)uxEndAddress;
    pxEnd->xBlockSize = 0;
    pxEnd->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(NULL);

    /* Bloc initial : heap moins l’espace de pxEnd */
    pxFirstFreeBlock = (BlockLink_t*)uxStartAddress;
    pxFirstFreeBlock->xBlockSize =
        (size_t)(uxEndAddress - (portPOINTER_SIZE_TYPE)pxFirstFreeBlock);
    pxFirstFreeBlock->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(pxEnd);

    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
}

static void prvInsertBlockIntoFreeList(BlockLink_t* pxBlockToInsert)
{
    BlockLink_t* pxIterator;
    uint8_t* puc;

    for(pxIterator = &xStart;
        heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock) < pxBlockToInsert;
        pxIterator = heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock)) {

    }

    if(pxIterator != &xStart) {
        heapVALIDATE_BLOCK_POINTER(pxIterator);
    }

    puc = (uint8_t*)pxIterator;

    if((puc + pxIterator->xBlockSize) == (uint8_t*)pxBlockToInsert) {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    } else {
        mtCOVERAGE_TEST_MARKER();
    }

    puc = (uint8_t*)pxBlockToInsert;

    if((puc + pxBlockToInsert->xBlockSize) ==
       (uint8_t*)heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock)) {
        if(heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock) != pxEnd) {

            pxBlockToInsert->xBlockSize +=
                heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock)->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock =
                heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock)->pxNextFreeBlock;
        } else {
            pxBlockToInsert->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(pxEnd);
        }
    } else {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* Bloc fusionné des deux côtés : conserver next pour éviter auto-lien */
    if(pxIterator != pxBlockToInsert) {
        pxIterator->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(pxBlockToInsert);
    } else {
        mtCOVERAGE_TEST_MARKER();
    }
}

void vPortGetHeapStats(HeapStats_t* pxHeapStats) {
    BlockLink_t* pxBlock;
    size_t
        xBlocks = 0,
        xMaxSize = 0,
        xMinSize =
            portMAX_DELAY; /* portMAX_DELAY fournit une valeur maximale portable */

    vTaskSuspendAll();
    {
        pxBlock = heapPROTECT_BLOCK_POINTER(xStart.pxNextFreeBlock);

        /* pxBlock NULL avant la première allocation */
        if(pxBlock != NULL) {
            while(pxBlock != pxEnd) {
                xBlocks++;

                if(pxBlock->xBlockSize > xMaxSize) {
                    xMaxSize = pxBlock->xBlockSize;
                }

                if(pxBlock->xBlockSize < xMinSize) {
                    xMinSize = pxBlock->xBlockSize;
                }

                pxBlock = heapPROTECT_BLOCK_POINTER(pxBlock->pxNextFreeBlock);
            }
        }
    }
    (void)xTaskResumeAll();

    pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
    pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
    pxHeapStats->xNumberOfFreeBlocks = xBlocks;

    taskENTER_CRITICAL();
    {
        pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining;
        pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations;
        pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees;
        pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining;
    }
    taskEXIT_CRITICAL();
}

/* Reset obligatoire avant redémarrage du scheduler */
void vPortHeapResetState(void) {
    pxEnd = NULL;

    xFreeBytesRemaining = (size_t)0U;
    xMinimumEverFreeBytesRemaining = (size_t)0U;
    xNumberOfSuccessfulAllocations = (size_t)0U;
    xNumberOfSuccessfulFrees = (size_t)0U;
}
