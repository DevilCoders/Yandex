#include "qd_scheme.h"

#include <kernel/querydata/common/querydata_traits.h>
#include <kernel/querydata/idl/querydata_structs_client.pb.h>
#include <util/stream/output.h>

namespace NQueryData {

    static inline void FillScheme(NSc::TValue& v, const TFactor& f) {
        NSc::TValue& vv = v.GetOrAdd(f.GetName());
        if (f.HasStringValue())
            vv = f.GetStringValue();
        else if (f.HasFloatValue())
            vv = f.GetFloatValue();
        else if (f.HasIntValue())
            vv = f.GetIntValue();
    }

    static void FillScheme(NSc::TValue& v, const TSourceFactors& f) {
        NSc::TValue vv = v;

        if (!f.GetCommon()) {
            if (SkipNormalizationInScheme(f.GetSourceKeyType()))
                return;

            for (ui32 i = 0, sz = f.SourceSubkeysSize(); i < sz; ++i) {
                const TSourceSubkey& sk = f.GetSourceSubkeys(i);
                if (SkipNormalizationInScheme(sk.GetType()))
                    return;
            }

            if (NormalizationNeedsExplicitKeyInResult(f.GetSourceKeyType()) || f.GetSourceKeyTraits().GetIsPrioritized())
                vv = v.GetOrAdd(f.GetSourceKey());

            for (ui32 i = 0, sz = f.SourceSubkeysSize(); i < sz; ++i) {
                const TSourceSubkey& sk = f.GetSourceSubkeys(i);
                if (NormalizationNeedsExplicitKeyInResult(sk.GetType()) || sk.GetTraits().GetMustBeInScheme())
                    vv = vv.GetOrAdd(sk.GetKey());
            }

            vv.SetDict();
        }

        for (ui32 i = 0, sz = f.FactorsSize(); i < sz; ++i)
            FillScheme(vv, f.GetFactors(i));

        if (f.HasJson()) {
            vv.MergeUpdateJson(f.GetJson());
        }
    }

    void QueryData2Scheme(NSc::TValue& v, const TQueryData& qd, bool realtime) {
        for (ui32 i = 0, sz = qd.SourceFactorsSize(); i < sz; ++i) {
            const TSourceFactors& sf = qd.GetSourceFactors(i);
            if (realtime != sf.GetRealTime())
                continue;

            NSc::TValue sfv = v.GetOrAdd(sf.GetSourceName());
            FillScheme(sfv.GetOrAdd(sf.GetCommon() ? SC_COMMON : SC_VALUES), sf);
            sfv.GetOrAdd(SC_TIMESTAMP) = Max<i64>(sfv.Get(SC_TIMESTAMP).GetIntNumber(), GetTimestampFromVersion(sf.GetVersion()));
        }
    }

    NSc::TValue QueryData2Scheme(const TQueryData& qd, bool realtime) {
        NSc::TValue val;
        QueryData2Scheme(val, qd, realtime);
        return val;
    }

}
