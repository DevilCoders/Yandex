#pragma once

#include <util/generic/fwd.h>


namespace NItdItp {


    struct TSiteRecommendationSettingLimits {
        static constexpr size_t MaxTags = 64;
        static constexpr size_t MaxTagLength = 128;

        static constexpr size_t MaxCategories = 64;
        static constexpr size_t MaxCategoryLength = 128;
    };


}
