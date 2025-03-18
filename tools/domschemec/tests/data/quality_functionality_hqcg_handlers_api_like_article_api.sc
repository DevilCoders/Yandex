namespace NHQCG::NSystemApi;

struct TArticleScore {
    article_id      : string (required);
    version_id      : string (required);
    author_puid     : string (required); // tmp requirements
    score           : double (required);
    yesterday_likes : i64;
};

struct TArticlesScoresBatch {
    articles : [TArticleScore];
};
