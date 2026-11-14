/* Array non type-safe sans append/remove/take */
/* Init réserve n éléments et appelle ctor in-place si fourni */
/* Reset appelle destructor par élément ; free déclenche reset */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SimpleArray SimpleArray;

typedef void SimpleArrayData;
typedef void SimpleArrayElement;

typedef void (*SimpleArrayInit)(SimpleArrayElement* elem);
typedef void (*SimpleArrayReset)(SimpleArrayElement* elem);
typedef void (*SimpleArrayCopy)(SimpleArrayElement* elem, const SimpleArrayElement* other);

/** Simple Array configuration structure. Defined per type. */
typedef struct {
    SimpleArrayInit init; /* Méthode initialisation : constructeur sur place */
    SimpleArrayReset reset;
    SimpleArrayCopy copy; /**< Copy (custom copy-constructor) method. */
    const size_t type_size; /* Taille du type en octets */
} SimpleArrayConfig;

SimpleArray* simple_array_alloc(const SimpleArrayConfig* config);

void simple_array_free(SimpleArray* instance);

/* Init callback par élément si défini ; sinon données zéro */

void simple_array_init(SimpleArray* instance, uint32_t count);

/* Reset callback par élément si défini ; puis libération données */

void simple_array_reset(SimpleArray* instance);

/* Copy callback par élément si défini ; sinon memcpy */

void simple_array_copy(SimpleArray* instance, const SimpleArray* other);

bool simple_array_is_equal(const SimpleArray* instance, const SimpleArray* other);

uint32_t simple_array_get_count(const SimpleArray* instance);

/* Index strictement inférieur au nombre éléments requis */

SimpleArrayElement* simple_array_get(SimpleArray* instance, uint32_t index);

/* Index strictement inférieur au nombre éléments requis */

const SimpleArrayElement* simple_array_cget(const SimpleArray* instance, uint32_t index);

SimpleArrayData* simple_array_get_data(SimpleArray* instance);

const SimpleArrayData* simple_array_cget_data(const SimpleArray* instance);

// Standard preset configurations

/* Configuration array octets uint8_t */
extern const SimpleArrayConfig simple_array_config_uint8_t;

#ifdef __cplusplus
}
#endif
