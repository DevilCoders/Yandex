#include "rating.h"
#include "schemaorg_parse.h"

namespace NSchemaOrg {
    TWtringBuf TRating::GetRatingValue() const {
        return ParseRating(RatingValue);
    }

    TWtringBuf TRating::GetBestRating() const {
        return ParseRating(BestRating);
    }

    TWtringBuf TRating::GetWorstRating() const {
        return ParseRating(WorstRating);
    }

    TWtringBuf TRating::GetRatingCount() const {
        return ParseRating(RatingCount);
    }
}
