#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PulseJoiner PulseJoiner;

PulseJoiner* pulse_joiner_alloc(void);

void pulse_joiner_free(PulseJoiner* pulse_joiner);

/* Premier négatif omis ; true exige pop immédiat ; unités timer ticks */

bool pulse_joiner_push_pulse(PulseJoiner* pulse_joiner, bool polarity, size_t period, size_t pulse);

/* Pop uniquement après push=true ; unités timer ticks */

void pulse_joiner_pop_pulse(PulseJoiner* pulse_joiner, size_t* period, size_t* pulse);

#ifdef __cplusplus
}
#endif
