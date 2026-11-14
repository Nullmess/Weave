#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Version Version;

/* Handle version conservable ; getters utilisent version courante si NULL */

const Version* version_get(void);

/* NULL : version logiciel exécuté */

const char* version_get_githash(const Version* v);

/* NULL : version logiciel exécuté */

const char* version_get_gitbranch(const Version* v);

/* NULL : version logiciel exécuté */

const char* version_get_gitbranchnum(const Version* v);

/* NULL : version logiciel exécuté */

const char* version_get_builddate(const Version* v);

/* Dernier tag Git ; v NULL pour firmware courant */

const char* version_get_version(const Version* v);

/* Hardware target build ; v NULL pour firmware courant */

uint8_t version_get_target(const Version* v);

/* Dirty si modifications non commitées ; v NULL pour firmware courant */

bool version_get_dirty_flag(const Version* v);

/* Origine via FIRMWARE_ORIGIN : Official ou nom du fork */

const char* version_get_firmware_origin(const Version* v);

const char* version_get_git_origin(const Version* v);

#ifdef __cplusplus
}
#endif
