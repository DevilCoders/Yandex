namespace NRecommender::NSCProto;

struct TRecommendationsStats {
    left_urls_total (cppname = LeftUrlsTotal, required) : ui64 (default = 0);
    right_urls_total (cppname = RightUrlsTotal, required) : ui64 (default = 0);
    right_urls_with_empty_title (cppname = RightUrlsWithEmptyTitle) : ui64 (default = 0);
    right_urls_with_empty_annotation (cppname = RightUrlsWithEmptyAnnotation) : ui64 (default = 0);
    full_value_right_urls (cppname = FullValueRightUrls) : ui64 (default = 0); // Count of right urls with non empty title and image
    full_value_right_urls_stats (cppname = FullValueRightUrlsStats) : {ui64 -> ui64}; // Count of recommendations to how many cases with this count of full-value recommendations
};
