namespace NBASSSkill;

struct TCardBlock {
    struct TCase {
        idx: string (required);
        activation: string (required);
        description: string (required);
        logo: string (required);
        recommendation_type: string (required);
        recommendation_source: string (required);
    };

    cases : [ TCase ] (required);
    store_url: string (required);
    recommendation_type: string (required);
    recommendation_source: string (required);
};

