#include "videoobj.h"

#include <library/cpp/string_utils/url/url.h>

namespace NSchemaOrg {
    TVideoHostWhiteList::TVideoHostWhiteList() {
        // this mapping is gotten from http://video.yandex.ru/static/ext-hostings-info.xml
        Host2VideoHost["youtube.com"] = "www.youtube.com";
        Host2VideoHost["vimeo.com"] = "vimeo.com";
        Host2VideoHost["dailymotion.com"] = "www.dailymotion.com";
    }

    TString TVideoHostWhiteList::GetHostKey(TStringBuf hostName) {
        return TString(CutWWWPrefix(hostName));
    }

    bool TVideoHostWhiteList::HasHost(TStringBuf hostName) const {
        TString hostWithoutWWW = GetHostKey(hostName);
        return Host2VideoHost.contains(hostWithoutWWW);
    }

    TString TVideoHostWhiteList::GetVideoHost(TStringBuf hostName) const {
        TString hostWithoutWWW = GetHostKey(hostName);
        auto it = Host2VideoHost.find(hostWithoutWWW);
        if (it == Host2VideoHost.end()) {
            return TString();
        }
        return it->second;
    }
}
