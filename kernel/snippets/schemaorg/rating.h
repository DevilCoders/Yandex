#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace NSchemaOrg {
    class TRating {
    public:
        TUtf16String RatingValue;
        TUtf16String BestRating;
        TUtf16String WorstRating;
        TUtf16String RatingCount;

        TWtringBuf GetRatingValue() const;
        TWtringBuf GetBestRating() const;
        TWtringBuf GetWorstRating() const;
        TWtringBuf GetRatingCount() const;
    };
}
