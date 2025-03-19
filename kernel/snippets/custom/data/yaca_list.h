#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NRegExp {
    class TFsm;
}

class IInputStream;

namespace NSnippets
{
    class TForcedYacaUrls {
    private:
        THolder<NRegExp::TFsm> UrlChecker;
    public:
        void LoadFromStream(IInputStream& in);
        void LoadDefault();
        void LoadFromFile(const TString& file);
        bool Contains(const TString& url) const;
        static const TForcedYacaUrls& GetDefault();
    };
}
