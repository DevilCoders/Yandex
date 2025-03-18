namespace NRtx::NGames;

struct TGame {
    id (cppname = Id): string;
    title (cppname = Title): string;
    categoryName (cppname = Categories): struct {
        categories (cppname = List): [string];
    };
    commonRating (cppname = CommonRating): double;
    thumbnailUrl (cppname = ThumbnailUrl): string;
};

using TExport = [TGame];
