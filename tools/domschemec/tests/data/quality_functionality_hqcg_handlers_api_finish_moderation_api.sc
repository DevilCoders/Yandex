namespace NHQCG::NSystemApi;

struct TFinishModeration {
    author_puid : ui64   (required);
    article_id  : string (required);
    version_id  : string (required);
    contest_id  : string (required);
    turbo_url   : string (required);
    status      : string (required, allowed = ["accept", "decline"]);
    reason      : string;
};
