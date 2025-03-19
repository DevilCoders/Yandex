#include "alice_search_request_info.h"

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/generic/algorithm.h>


namespace {
    constexpr TStringBuf ALICE_MUSIC_SEARCH_USER_REQUEST_SUFFIX = "host:music.yandex.ru";
    constexpr TStringBuf ALICE_REQINFO_VALUE_PREFIX = "assistant.yandex";
} // namespace


namespace NAliceSearch {
    bool IsAliceMusicScenarioRequest(TStringBuf userRequest, const TCgiParameters& cgi) {
        // TODO(gotmanov@, SEARCH-9708)
        // This part is fragile ; rewrite ASAP using data from MEGAMIND-555
        // instead of reqinfo.
        auto reqinfoRange = cgi.Range(TStringBuf("reqinfo"));

        const bool hasAssistantReqinfo = AnyOf(
            reqinfoRange.begin(),
            reqinfoRange.end(),
            [](const auto& x) {
                return x.StartsWith(ALICE_REQINFO_VALUE_PREFIX);
            });

        return hasAssistantReqinfo && userRequest.EndsWith(ALICE_MUSIC_SEARCH_USER_REQUEST_SUFFIX);
    }
} // NAliceSearch
