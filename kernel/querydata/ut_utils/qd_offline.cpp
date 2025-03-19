#include "qd_offline.h"
#include "qd_ut_utils.h"

#include <kernel/querydata/scheme/qd_scheme.h>
#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <library/cpp/json/json_prettifier.h>

namespace NQueryData {

    void PrintSearchedQuery(IOutputStream& out, TQueryDatabase& db, TRequestRec& rec, TStringBuf query, EPrintQueryMode p) {
        bool compact = p & PQM_COMPACT;
        bool skipnorm = p & PQM_SKIP_NORM;
        bool json = p & PQM_JSON;

        NTests::MockQueryDataRequest(rec, query);

        out << query << (compact ? '\t' : '\n');

        TQueryData qd;

        db.GetQueryData(rec, qd, skipnorm);
        out << HumanReadableQueryData(qd, compact);

        if (json) {
            NSc::TValue v;
            QueryData2Scheme(v, qd);
            TString sjson = v.ToJson(true);
            if (compact) {
                out << '\t' << sjson;
            } else {
                NJson::PrettifyJson(sjson, out);
            }
        }

        out << '\n';

        if (!compact) {
            out << '\n';
        }
    }

}
