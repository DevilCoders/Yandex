#pragma once

#include <kernel/querydata/server/qd_server.h>
#include <util/stream/output.h>

namespace NQueryData {

    enum EPrintQueryMode {
        PQM_NONE = 0,
        PQM_SKIP_NORM = 1,
        PQM_COMPACT = 2,
        PQM_JSON = 4,
    };

    void PrintSearchedQuery(IOutputStream& out, TQueryDatabase& db, TRequestRec& rec, TStringBuf query, EPrintQueryMode p);

}
