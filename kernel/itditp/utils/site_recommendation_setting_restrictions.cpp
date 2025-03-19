#include "site_recommendation_setting_restrictions.h"

#include "site_recommendation_setting_limits.h"

#include <util/generic/algorithm.h>



namespace NItdItp {

    bool IsCorrectTagString(const TStringBuf& str) {
        return str.size() <= TSiteRecommendationSettingLimits::MaxTagLength && AllOf(str.begin(), str.end(), [](char letter) {
            return std::isalnum(letter) || letter == '_' || letter == '-';
        });
    }

    bool IsCorrectCategoryString(const TStringBuf& str) {
        return str.size() <= TSiteRecommendationSettingLimits::MaxCategoryLength && AllOf(str.begin(), str.end(), [](char letter) {
            return letter != '\n' && letter != '\0' && letter != ',' && letter != '\t';
        });
    }

}
