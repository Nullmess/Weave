#pragma once

#include <m-tuple.h>
#include <m-array.h>
#include <m-algo.h>
#include <m-funcobj.h>

typedef enum {
    SubGhzFrequencyAnalyzerLogOrderBySeqDesc,
    SubGhzFrequencyAnalyzerLogOrderBySeqAsc,
    SubGhzFrequencyAnalyzerLogOrderByCountDesc,
    SubGhzFrequencyAnalyzerLogOrderByCountAsc,
    SubGhzFrequencyAnalyzerLogOrderByRSSIDesc,
    SubGhzFrequencyAnalyzerLogOrderByRSSIAsc,
    SubGhzFrequencyAnalyzerLogOrderByFrequencyDesc,
    SubGhzFrequencyAnalyzerLogOrderByFrequencyAsc,
} SubGhzFrequencyAnalyzerLogOrderBy;

const char*
    subghz_frequency_analyzer_log_get_order_name(SubGhzFrequencyAnalyzerLogOrderBy order_by);

TUPLE_DEF2( //-V1048
    SubGhzFrequencyAnalyzerLogItem,
    (seq, uint8_t),
    (frequency, uint32_t),
    (count, uint8_t),
    (rssi_max, uint8_t))

#define M_OPL_SubGhzFrequencyAnalyzerLogItem_t() \
    TUPLE_OPLIST(                                \
        SubGhzFrequencyAnalyzerLogItem,          \
        M_DEFAULT_OPLIST,                        \
        M_DEFAULT_OPLIST,                        \
        M_DEFAULT_OPLIST,                        \
        M_DEFAULT_OPLIST)

ARRAY_DEF(SubGhzFrequencyAnalyzerLogItemArray, SubGhzFrequencyAnalyzerLogItem_t) //-V779 //-V658
#define M_OPL_SubGhzFrequencyAnalyzerLogItemArray_t() \
    ARRAY_OPLIST(SubGhzFrequencyAnalyzerLogItemArray, M_OPL_SubGhzFrequencyAnalyzerLogItem_t())
ALGO_DEF(SubGhzFrequencyAnalyzerLogItemArray, SubGhzFrequencyAnalyzerLogItemArray_t)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
FUNC_OBJ_INS_DEF(
    SubGhzFrequencyAnalyzerLogItemArray_compare_by  ,
    SubGhzFrequencyAnalyzerLogItemArray_cmp_obj  ,
    (a,
     b) /* Noms paramètres, types hérités de l’interface */
    ,
    {

        if(self->order_by == SubGhzFrequencyAnalyzerLogOrderByFrequencyAsc) {
            return a->frequency < b->frequency ? -1 : a->frequency > b->frequency;
        }
        if(self->order_by == SubGhzFrequencyAnalyzerLogOrderByFrequencyDesc) {
            return a->frequency > b->frequency ? -1 : a->frequency < b->frequency;
        }
        if(self->order_by == SubGhzFrequencyAnalyzerLogOrderByRSSIAsc) {
            return a->rssi_max < b->rssi_max ? -1 : a->rssi_max > b->rssi_max;
        }
        if(self->order_by == SubGhzFrequencyAnalyzerLogOrderByRSSIDesc) {
            return a->rssi_max > b->rssi_max ? -1 : a->rssi_max < b->rssi_max;
        }
        if(self->order_by == SubGhzFrequencyAnalyzerLogOrderByCountAsc) {
            return a->count < b->count ? -1 : a->count > b->count;
        }
        if(self->order_by == SubGhzFrequencyAnalyzerLogOrderByCountDesc) {
            return a->count > b->count ? -1 : a->count < b->count;
        }
        if(self->order_by == SubGhzFrequencyAnalyzerLogOrderBySeqAsc) {
            return a->seq < b->seq ? -1 : a->seq > b->seq;
        }

        return a->seq > b->seq ? -1 : a->seq < b->seq;
    },

    (order_by, SubGhzFrequencyAnalyzerLogOrderBy))
#define M_OPL_SubGhzFrequencyAnalyzerLogItemArray_compare_by_t() \
    FUNC_OBJ_INS_OPLIST(SubGhzFrequencyAnalyzerLogItemArray_compare_by, M_DEFAULT_OPLIST)
#pragma GCC diagnostic pop
