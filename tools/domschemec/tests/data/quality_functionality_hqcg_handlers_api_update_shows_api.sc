namespace NHQCG::NSystemApi;

struct TArticleShows {
    puid       : ui64   (required);
    article_id : string (required);
    version_id : string (required);
    url        : string (required);
    yesterday  : ui64;
    week       : ui64;
    month      : ui64;
    total      : ui64;
};

struct TArticlesShowsBatch {
    articles : [TArticleShows];
};
