namespace NHQCG::NSystemApi;

struct TBrokenArticle {
    author_puid : string (required);
    article_id  : string (required);

    broken_delay_seconds : ui32 (default = 86400);
};
