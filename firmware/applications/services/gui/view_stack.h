/* Draw dans ordre ajout ; input en ordre inverse */
/* Input consommé non transmis aux couches inférieures */
/* Vues interdites simultanément dans ViewDispatcher/ViewHolder */

#pragma once

#include <stdbool.h>
#include "view.h"

#ifdef __cplusplus
extern "C" {
#endif

/** ViewStack, anonymous type. */
typedef struct ViewStack ViewStack;

ViewStack* view_stack_alloc(void);

void view_stack_free(ViewStack* view_stack);

/* Vue ViewStack à connecter au view manager */

View* view_stack_get_view(ViewStack* view_stack);

/* Ajout au sommet du ViewStack */

void view_stack_add_view(ViewStack* view_stack, View* view);

/* Suppression ignorée si vue absente */

void view_stack_remove_view(ViewStack* view_stack, View* view);

#ifdef __cplusplus
}
#endif
