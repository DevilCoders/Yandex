#pragma once

#include "lcookie.h"

#include <util/generic/hash.h>

namespace NLCookie {
    class TFileKeyReader: public IKeychain {
    public:
        TFileKeyReader(const TString& path) {
            Load(path);
        }

        TStringBuf GetKey(const TStringBuf& keyId) const override;

    private:
        void Load(const TString& path);

        typedef THashMap<TStringBuf, TStringBuf> TKeyMap;

        TString keyStore;
        TKeyMap keyMap;
    };

}
