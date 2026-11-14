/* Header interne exclu des API publiques */

#include <stdint.h>
#include <stdbool.h>

#define DIGITAL_SIGNAL_T_TIM      1562 /**< 15.625 ns *100 */
#define DIGITAL_SIGNAL_T_TIM_DIV2 (DIGITAL_SIGNAL_T_TIM / 2) /**< 15.625 ns / 2 *100 */

/**
 * @brief DigitalSignal structure type.
 */
struct DigitalSignal {
    bool start_level;
    uint32_t size;
    uint32_t max_size;
    int32_t remainder; /* Reste conversion périodes vers ticks timer */
    uint32_t data[];
};
