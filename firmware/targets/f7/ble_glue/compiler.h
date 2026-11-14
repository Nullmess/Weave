#pragma once

#ifndef __PACKED_STRUCT
#define __PACKED_STRUCT PACKED(struct)
#endif

#ifndef __PACKED_UNION
#define __PACKED_UNION PACKED(union)
#endif

#if defined(__ICCARM__) || defined(__IAR_SYSTEMS_ASM__)

#ifndef __WEAK
#define __WEAK __weak
#endif

#define QUOTE_(a) #a

/* PACKED(struct/union) pour packing */

#define PACKED(decl) __packed decl

/* SECTION(".name") pour placement data/code */

#define SECTION(name) _Pragma(QUOTE_(location = name))

/* ALIGN_DEF(n) pour alignement */

#define ALIGN_DEF(v) _Pragma(QUOTE_(data_alignment = v))

/* NO_INIT(type variable/array) sans initialisation */

#define NO_INIT(var) __no_init var

#else
#ifdef __GNUC__

#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

/* PACKED(struct/union) pour packing */

#define PACKED(decl)  decl __attribute__((packed))

/* SECTION(".name") pour placement data/code */

#define SECTION(name) __attribute__((section(name)))

/* ALIGN_DEF(n) pour alignement */

#define ALIGN_DEF(N)  __attribute__((aligned(N)))

/* NO_INIT(type variable/array) sans initialisation */

#define NO_INIT(var)  var __attribute__((section(".noinit")))

#else
#ifdef __CC_ARM

#ifndef __WEAK
#define __WEAK __weak
#endif

/* PACKED(struct/union) pour packing */

#define PACKED(decl)  decl __attribute__((packed))

/* SECTION(".name") pour placement data/code */

#define SECTION(name) __attribute__((section(name)))

/* ALIGN_DEF(n) pour alignement */

#define ALIGN_DEF(N)  __attribute__((aligned(N)))

/* NO_INIT(type variable/array) sans initialisation */

#define NO_INIT(var)  var __attribute__((section("NoInit")))

#else

#error Neither ICCARM, CC ARM nor GNUC C detected. Define your macros.

#endif
#endif
#endif
