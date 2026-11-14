
#pragma once

#include "base.h"
#include "common_defines.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Certaines opérations exigent un thread arrêté */
typedef enum {
    FuriThreadStateStopped, /* Événement depuis system init après nettoyage TCB ; libération permise */
    FuriThreadStateStopping, /* Événement depuis le thread enfant */
    FuriThreadStateStarting, /* Événement depuis le thread parent */
    FuriThreadStateRunning, /* Événement depuis le thread enfant */
} FuriThreadState;

typedef enum {
    FuriThreadPriorityIdle = 0,
    FuriThreadPriorityInit = 4,
    FuriThreadPriorityLowest = 14,
    FuriThreadPriorityLow = 15,
    FuriThreadPriorityNormal = 16, /* Priorité système par défaut */
    FuriThreadPriorityHigh = 17,
    FuriThreadPriorityHighest = 18,
    FuriThreadPriorityIsr =
        (FURI_CONFIG_THREAD_MAX_PRIORITIES - 1), /* ISR différée : priorité maximale */
} FuriThreadPriority;

typedef struct FuriThread FuriThread;

typedef struct FuriThreadList FuriThreadList;

typedef void* FuriThreadId;

typedef int32_t (*FuriThreadCallback)(void* context);

/* Handler stdout : traiter toutes les données avant retour */
typedef void (*FuriThreadStdoutWriteCallback)(const char* data, size_t size, void* context);

/* Lecture bornée par size ; timeout en ticks */
typedef size_t (
    *FuriThreadStdinReadCallback)(char* buffer, size_t size, FuriWait timeout, void* context);

typedef void (*FuriThreadStateCallback)(FuriThread* thread, FuriThreadState state, void* context);

/* Retour true si signal traité ; arg peut être NULL */
typedef bool (*FuriThreadSignalCallback)(uint32_t signal, void* arg, void* context);

FuriThread* furi_thread_alloc(void);

/* Service : callback sans retour, sans join/libération */
/* Stack immuable en bytes ; name peut être NULL */
FuriThread* furi_thread_alloc_service(
    const char* name,
    uint32_t stack_size,
    FuriThreadCallback callback,
    void* context);

/* Name peut être NULL ; stack en bytes */
FuriThread* furi_thread_alloc_ex(
    const char* name,
    uint32_t stack_size,
    FuriThreadCallback callback,
    void* context);

/* Thread arrêté requis ; consulter les contraintes de join */
void furi_thread_free(FuriThread* thread);

/* Thread arrêté requis ; name peut être NULL */
void furi_thread_set_name(FuriThread* thread, const char* name);

/* Thread arrêté requis ; appid peut être NULL */
/* Appid commun aux threads d’une application, non unique */
void furi_thread_set_appid(FuriThread* thread, const char* appid);

/* Thread arrêté requis ; stack en bytes, immuable pour services */
void furi_thread_set_stack_size(FuriThread* thread, size_t stack_size);

/* Thread arrêté requis */
void furi_thread_set_callback(FuriThread* thread, FuriThreadCallback callback);

/* Thread arrêté requis ; context peut être NULL */
void furi_thread_set_context(FuriThread* thread, void* context);

/* Thread arrêté requis */
void furi_thread_set_priority(FuriThread* thread, FuriThreadPriority priority);

FuriThreadPriority furi_thread_get_priority(FuriThread* thread);

void furi_thread_set_current_priority(FuriThreadPriority priority);

FuriThreadPriority furi_thread_get_current_priority(void);

/* Thread arrêté requis */
void furi_thread_set_state_callback(FuriThread* thread, FuriThreadStateCallback callback);

/* Thread arrêté requis ; context peut être NULL */
void furi_thread_set_state_context(FuriThread* thread, void* context);

FuriThreadState furi_thread_get_state(FuriThread* thread);

/* Appel externe : thread arrêté requis ; context peut être NULL */
void furi_thread_set_signal_callback(
    FuriThread* thread,
    FuriThreadSignalCallback callback,
    void* context);

/* Retour NULL si aucun callback enregistré */
FuriThreadSignalCallback furi_thread_get_signal_callback(const FuriThread* thread);

/* Arg peut être NULL */
bool furi_thread_signal(const FuriThread* thread, uint32_t signal, void* arg);

/* Thread arrêté requis */
void furi_thread_start(FuriThread* thread);

/* Join exige retour du callback ; renvoie toujours true */
/* La tâche Idle doit s’exécuter, sinon attente infinie */
bool furi_thread_join(FuriThread* thread);

/* Retour NULL si thread non démarré */
FuriThreadId furi_thread_get_id(FuriThread* thread);

/* Thread arrêté requis */
void furi_thread_enable_heap_trace(FuriThread* thread);

/* Thread arrêté requis */
void furi_thread_disable_heap_trace(FuriThread* thread);

/* Traçage heap activé requis ; résultat en bytes */
size_t furi_thread_get_heap_size(FuriThread* thread);

/* Thread arrêté requis ; retour du callback */
int32_t furi_thread_get_return_code(FuriThread* thread);

FuriThreadId furi_thread_get_current_id(void);

/* Retour NULL si le thread n’appartient pas à Furi */
FuriThread* furi_thread_get_current(void);

void furi_thread_yield(void);

/* Retour flags précédents combinés aux nouveaux flags */
uint32_t furi_thread_flags_set(FuriThreadId thread_id, uint32_t flags);

/* Retour flags avant effacement */
uint32_t furi_thread_flags_clear(uint32_t flags);

uint32_t furi_thread_flags_get(void);

/* Timeout en ms ; FuriWaitForever pour attente infinie */
/* Retour flags reçus et flags d’erreur ; voir FuriFlag */
uint32_t furi_thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout);

/* Retour true si succès */
bool furi_thread_enumerate(FuriThreadList* thread_list);

/* Nom terminé par NUL ou NULL */
const char* furi_thread_get_name(FuriThreadId thread_id);

/* Appid terminé par NUL */
const char* furi_thread_get_appid(FuriThreadId thread_id);

uint32_t furi_thread_get_stack_space(FuriThreadId thread_id);

void furi_thread_get_stdout_callback(FuriThreadStdoutWriteCallback* callback, void** context);

void furi_thread_get_stdin_callback(FuriThreadStdinReadCallback* callback, void** context);

/* Callback NULL pour désactiver stdout */
void furi_thread_set_stdout_callback(FuriThreadStdoutWriteCallback callback, void* context);

/* Callback NULL pour désactiver stdin */
void furi_thread_set_stdin_callback(FuriThreadStdinReadCallback callback, void* context);

/* Retour nombre de bytes effectivement écrits */
size_t furi_thread_stdout_write(const char* data, size_t size);

int32_t furi_thread_stdout_flush(void);

/* Timeout en ticks ; retour nombre de bytes lus */
size_t furi_thread_stdin_read(char* buffer, size_t size, FuriWait timeout);

/* Lecture ultérieure dans le même ordre que l’insertion */
void furi_thread_stdin_unread(char* buffer, size_t size);

void furi_thread_suspend(FuriThreadId thread_id);

void furi_thread_resume(FuriThreadId thread_id);

bool furi_thread_is_suspended(FuriThreadId thread_id);

#ifdef __cplusplus
}
#endif
