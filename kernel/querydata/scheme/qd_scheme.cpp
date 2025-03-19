#include "qd_scheme.h"

#include <kernel/querydata/idl/querydata_structs_client.pb.h>

namespace NQueryData {

    void DoFillValue(NSc::TValue& val, const TSourceFactors& sf, bool binfactor) {
        for (ui32 i = 0, sz = sf.FactorsSize(); i < sz; ++i) {
            const TFactor& f = sf.GetFactors(i);
            NSc::TValue& vv = val.GetOrAdd(f.GetName());
            if (f.HasStringValue()) {
                vv = f.GetStringValue();
            } else if (f.HasFloatValue()) {
                vv = f.GetFloatValue();
            } else if (f.HasIntValue()) {
                vv = f.GetIntValue();
            } else if (binfactor && f.HasBinaryValue()) {
                vv = f.GetBinaryValue();
            }
        }

        val.MergeUpdateJson(sf.GetJson());
    }

}
