

#ifndef _FF_INTEGER
#define _FF_INTEGER

#ifdef _WIN32

#include <windows.h>
#include <tchar.h>
typedef unsigned __int64 QWORD;

#else
#include <stdint.h>

/* Types obligatoirement 16 ou 32 bits */
typedef int16_t INT;
typedef uint16_t UINT;

/* Type obligatoirement 8 bits */
typedef uint8_t BYTE;

/* Types obligatoirement 16 bits */
typedef int16_t SHORT;
typedef uint16_t WORD;
typedef uint16_t WCHAR;

/* Types obligatoirement 32 bits */
typedef int32_t LONG;
typedef uint32_t DWORD;

/* Type 64 bits ; retirer pour compatibilité C89 */
typedef uint64_t QWORD;

#endif

#endif
