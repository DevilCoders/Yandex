namespace NCardsScheme;

struct TCardType {
    body : any;
};

struct TRelativeRestriction {
    mask : i64;
    time_start : ui64; // seconds since start of day
    duration : ui64; // seconds
};

struct TGeoRestriction {
     lat : double;
     lon : double;
     distance : double;
     relevance : i64;
     location_required : bool;
};

struct TCardRestrictions {
    start_time : string; // iso8601 date
    stop_time : string; // iso8601 date
    time_zone : string;
    week_days : [ TRelativeRestriction ];
    geo_id : any; // [ui32] | ui32 (can't express uinons in .sc yet)
    geo : TGeoRestriction;

    (validate) {
        if (HasWeekDays() && !HasTimeZone()) {
            AddError("Card with relative restrictions (\"week_days\") must have time_zone attr set");
        }
    };
};

struct TUserCard {
    card_type : string (required, cppname = Type);
    card_subtype : string (required, cppname = SubType);
    card_id : string (cppname = Id);
    idgen : string (cppname = IdGen);

    restrictions : TCardRestrictions (required);

    // used for card instantiation
    params : any;
};

struct TPushCard {
    card_id   : string (cppname = Id);
    type      : string;
    date_to   : ui32 (required); // epoch
    date_from : ui32;            // epoch
};

struct TPush {
    _id            : string (required, cppname = PushId);
    card           : TPushCard (required);
    sent_date_time : ui32 (required); // epoch
};
