#pragma once

#include <furi.h>

#include "loader.h"

#define LOADER_QUEUE_MAX_SIZE 4

typedef struct {
    char* name_or_path;
    char* args;
    LoaderDeferredLaunchFlag flags;
} LoaderDeferredLaunchRecord;

typedef struct {
    LoaderDeferredLaunchRecord items[LOADER_QUEUE_MAX_SIZE];
    size_t item_cnt;
} LoaderLaunchQueue;

/* Libère données internes de DeferredLaunchRecord */

void loader_queue_item_clear(LoaderDeferredLaunchRecord* item);

/* false si queue vide */

bool loader_queue_pop(LoaderLaunchQueue* queue, LoaderDeferredLaunchRecord* item);

/* false si queue pleine */

bool loader_queue_push(LoaderLaunchQueue* queue, LoaderDeferredLaunchRecord* item);

void loader_queue_clear(LoaderLaunchQueue* queue);
