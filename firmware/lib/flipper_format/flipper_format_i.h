#pragma once
#include <toolbox/stream/stream.h>
#include "flipper_format.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Accès stream interne réservé aux usages maîtrisés */

Stream* flipper_format_get_raw_stream(FlipperFormat* flipper_format);

#ifdef __cplusplus
}
#endif
