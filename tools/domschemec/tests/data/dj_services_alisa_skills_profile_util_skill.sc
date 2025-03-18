namespace NASSKILL;

struct TSkill {
    struct TLogo {
        prefix : string (default = "");
        avatarId : string (required);
    };

    id : string (required);
    generalId : string (required);
    examples : [ string ] (required);
    description : string (required);
    logo : TLogo (required);
    category : string (required);

    hideInStore : bool (default = false);
    isBanned : bool (default = false);
    onAir : bool (default = true);
    isRecommended : bool (default = true);
    isVip: bool (default = false);
    requirements: [ string ];
    surfaces: [ string ];
    look : string (default = "external");
    expFlag : string;
    timeFrom: string;
    timeTo: string;
    weekdays: [ string ];
    osVersions: [ string ];
    weight: double (default = 0);
    sampling_weight: double (default = 0);
    cards: [ string ];
    experiments: [ string ];
    source: string (default = "");
    isModification: bool (default = false);
};

