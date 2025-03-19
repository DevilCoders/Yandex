#pragma once

#include "qd_saas_key.h"

#include <util/generic/vector.h>

namespace NQueryData {
    class TQueryData;
}

namespace NQueryDataSaaS {

    class TSaaSReqProps;

    struct TProcessingStats {
        ui32 RecordsRejectedAsMalformed = 0; // Сколько записей было удалено как битые

        ui32 RecordsRejectedByRequest = 0; // Сколько было отфильтровано из-за несоответствия запросу

        ui32 RecordsRejectedByMerge = 0; // Сколько было отфильтровано мёржем с приоритетами

        ui32 RecordsAddedByFinalization = 0; // Сколько записей было добавлено после превращения масок в урлы

        TVector<TString> Errors;

        TProcessingStats& operator += (const TProcessingStats& other);
    };


    struct TSaaSDocument {
        TString Key;
        TVector<TString> Records;
    };


    TProcessingStats ProcessSaaSResponse(NQueryData::TQueryData&, const TSaaSDocument&, const TSaaSReqProps&);
}
