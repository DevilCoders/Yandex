#pragma once

#include <kernel/snippets/sent_match/sent_match.h>

namespace NSnippets {
    struct PixelLenResult {
    public:
        int StringNum;
        float LastStringFill;
        PixelLenResult(int stringNum, float lastStringFill)
            : StringNum(stringNum)
            , LastStringFill(lastStringFill)
        {
        }
    };

    PixelLenResult GetYandexPixelLen(const TSentsMatchInfo& sentsMatchInfo, size_t snipTextOffset);
    PixelLenResult GetGooglePixelLen(const TSentsMatchInfo& sentsMatchInfo, size_t snipTextOffset);

}
