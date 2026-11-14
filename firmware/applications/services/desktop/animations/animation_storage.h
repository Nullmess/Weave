#pragma once
#include <stdint.h>
#include <m-list.h>
#include "views/bubble_animation_view.h"

typedef struct StorageAnimation StorageAnimation;

typedef struct {
    const char* name;
    uint8_t min_butthurt;
    uint8_t max_butthurt;
    uint8_t min_level;
    uint8_t max_level;
    uint8_t weight;
} StorageAnimationManifestInfo;

LIST_DEF(StorageAnimationList, StorageAnimation*, M_PTR_OPLIST)
#define M_OPL_StorageAnimationList_t() LIST_OPLIST(StorageAnimationList)

/* Liste initialisée requise ; cache puis fallback animation interne si erreur */

void animation_storage_fill_animation_list(StorageAnimationList_t* list);

/* Cache si absent ; NULL si échec */

const BubbleAnimation* animation_storage_get_bubble_animation(StorageAnimation* storage_animation);

/* Cache si pas déjà effectué */

void animation_storage_cache_animation(StorageAnimation* storage_animation);

/* Recherche flash/SD ; NULL si absente */

StorageAnimation* animation_storage_find_animation(const char* name);

/* Metadata jamais NULL */

StorageAnimationManifestInfo* animation_storage_get_meta(StorageAnimation* storage_animation);

/* Libère storage_animation puis met à NULL */

void animation_storage_free_storage_animation(StorageAnimation** storage_animation);

/* Appeler une fois pour initialiser animations flash */

void animation_storage_initialize_internal_animations(void);
