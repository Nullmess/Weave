#include <flipper_application/api_hashtable/api_hashtable.h>
#include <flipper_application/api_hashtable/compilesort.hpp>

/* Resolver composite expose API internes aux plugins */

#include "nfc_app_api_table_i.h"

static_assert(!has_hash_collisions(nfc_app_api_table), "Detected API method hash collision!");

constexpr HashtableApiInterface nfc_application_hashtable_api_interface{
    {
        .api_version_major = 0,
        .api_version_minor = 0,
        /* generic resolver using pre-sorted array */
        .resolver_callback = &elf_resolve_from_hashtable,
    },
    /* Bornes de la table API applicative */
    nfc_app_api_table.cbegin(),
    nfc_app_api_table.cend(),
};

/* Resolver générique pour Composite API resolver */
extern "C" const ElfApiInterface* const nfc_application_api_interface =
    &nfc_application_hashtable_api_interface;
