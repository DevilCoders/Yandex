#pragma once

#include <util/generic/ptr.h>
#include <library/cpp/uri/http_url.h>

namespace re2 {
    class RE2;
}


namespace NMango {
    class TURLCanonizer {
    public:
        TURLCanonizer(bool withoutWWW = true, bool withoutScheme = false, bool dontStripSuffix = false);
        ~TURLCanonizer();
        TString Canonize(const TString &url) const;

    private:
        void CutUselessDocument(THttpURL& path) const;

        bool WithoutWWW;
        bool WithoutScheme;
        bool DontStripSuffix;

        THolder<re2::RE2> UselessDocSuffixRegExp;
    };
}
