namespace NHQCG::NSystemApi;

struct TUpdateTurboUrlResponse {
    struct TArticleStatus {
        article_id : string (required);
        title      : string (required);
        turbo_url  : string (required);
        status     : string (required);
        message    : string;
    };

    articles : [TArticleStatus];
};
