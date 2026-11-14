#include <flipper_application/api_hashtable/api_hashtable.h>
#include <flipper_application/api_hashtable/compilesort.hpp>

/* Table symboles privés pour le resolver composite */
#include "app_api_table_i.h"

static_assert(!has_hash_collisions(app_api_table), "Detected API method hash collision!");

constexpr HashtableApiInterface applicaton_hashtable_api_interface{
    {
        .api_version_major = 0,
        .api_version_minor = 0,
        /* Resolver générique avec table prétriée */
        .resolver_callback = &elf_resolve_from_hashtable,
    },
    /* Pointers vers les bornes de table API application */
    app_api_table.cbegin(),
    app_api_table.cend(),
};

/* Cast vers resolver générique pour composition des APIs */
extern "C" const ElfApiInterface* const application_api_interface =
    &applicaton_hashtable_api_interface;
