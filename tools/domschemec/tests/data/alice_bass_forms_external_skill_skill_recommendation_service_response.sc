namespace NBASSSkill;

struct TServiceResponse {
    struct TItem {
        id: string (required);
        activation: string (required);
        description: string (required);
        logo_prefix: string (required);
        logo_avatar_id: string (required);
        look: string (required);
    };

    items : [ TItem ] (required);
    recommendation_type: string (required);
    recommendation_source: string (required);
};

