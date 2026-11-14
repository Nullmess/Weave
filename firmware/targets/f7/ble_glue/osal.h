#pragma once

extern void* Osal_MemCpy(void* dest, const void* src, unsigned int size);

extern void* Osal_MemSet(void* ptr, int value, unsigned int size);

/* 0 si buffers identiques ; 1 sinon */

extern int Osal_MemCmp(const void* s1, const void* s2, unsigned int size);
