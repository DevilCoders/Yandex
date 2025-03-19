#include "qd_key_impl.h"
#include <util/string/cast.h>

namespace NQueryData {

    const TFactor* FindFactor(const TFactorsVec& fvec, TStringBuf name) {
        for (const auto& fac : fvec) {
            if (fac->GetName() == ToString(name)) {
                return fac;
            }
        }
        return nullptr;
    }

    TKey MakeKey(const TSourceFactors& src) {
        TKey k(src.GetCommon());

        k.Add(TSubkey{src.GetSourceKey(), src.GetSourceKeyType()});

        for (ui32 ski = 0, sksz = src.SourceSubkeysSize(); ski < sksz; ++ski) {
            const TSourceSubkey& sk = src.GetSourceSubkeys(ski);
            k.Add(TSubkey{sk.GetKey(), sk.GetType()});
        }

        return k;
    }

    TString TKey::Dump() const {
        TString res;

        if (Common) {
            res = "<COMMON>";
        }

        for (TSubkeys::const_iterator it = Subkeys.begin(); it != Subkeys.end(); ++it) {
            if (it != Subkeys.begin()) {
                res.append('/');
            }
            res.append(it->Key).append('(').append(GetNormalizationNameFromType(it->KeyType)).append(')');
        }

        return res;
    }

}
