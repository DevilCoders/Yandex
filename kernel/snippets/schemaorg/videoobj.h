#pragma once

#include <util/generic/string.h>
#include <util/generic/hash.h>

namespace NSchemaOrg {
    class TVideoHostWhiteList {
    private:
        typedef THashMap<TString, TString> THost2VideoHost;
        THost2VideoHost Host2VideoHost;

    private:
        static TString GetHostKey(TStringBuf hostName);

    public:
        TVideoHostWhiteList();
        bool HasHost(TStringBuf hostName) const;
        TString GetVideoHost(TStringBuf hostName) const;
    };

} // namespace NSchemaOrg
