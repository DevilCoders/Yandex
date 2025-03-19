#include "norm_structkey.h"

namespace NQueryData {

    static TStringBuf GetString(TDeque<TString>& pool, const NSc::TValue& elem) {
        if (elem.IsNumber()) {
            pool.push_back(ToString<double>(elem.GetNumber()));
            return pool.back();
        }

        return elem.GetString();
    }

    int GetStructKeys(TStringBufs& subkeys, TDeque<TString>& pool, const NSc::TValue& skeys, TStringBuf nspace) {
        int type = FAKE_KT_STRUCTKEY_ANY;

        if (!nspace) {
            return type;
        }

        const NSc::TValue& sub = skeys.Get(nspace);

        if (sub.IsArray()) {
            type = FAKE_KT_STRUCTKEY_ORDERED;
            const NSc::TArray& arr = sub.GetArray();
            for (NSc::TArray::const_iterator it = arr.begin(); it != arr.end(); ++it) {
                TStringBuf sb = GetString(pool, *it);

                if (!!sb) {
                    subkeys.push_back(sb);
                }
            }
        } else if (sub.IsDict()) {
            const NSc::TDict& dict = sub.GetDict();
            for (NSc::TDict::const_iterator it = dict.begin(); it != dict.end(); ++it) {
                TStringBuf sb = it->first;

                if (!!sb) {
                    subkeys.push_back(sb);
                }
            }
        } else {
            TStringBuf sb = GetString(pool, sub);
            if (!!sb) {
                subkeys.push_back(sb);
            }
        }
        return type;
    }

}
