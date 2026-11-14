#include "../ff.h"

#if _USE_LFN != 0

#if   _CODE_PAGE == 932	/* Japonais Shift-JIS */
#include "cc932.c"
#elif _CODE_PAGE == 936	/* Chinois simplifié GBK */
#include "cc936.c"
#elif _CODE_PAGE == 949	/* Coréen */
#include "cc949.c"
#elif _CODE_PAGE == 950	/* Chinois traditionnel Big5 */
#include "cc950.c"
#else					/* Charset à un seul byte */
#include "ccsbcs.c"
#endif

#endif
