namespace NApi;

// Weight modifiers
// @see https://st.yandex-team.ru/ASSISTANT-1338
struct TWeightModifier {
    name : string (required, cppname = WeightModName);
    k    : double (cppname = WeightModK, default = 0);
    boost: any (cppname = WeightModBoost);
};

struct TBlenderParams {
    zen_bellow_news : bool (default = false);

    N : ui16 (cppname = MaxMove, default = 1); // block can not be moved more than by "MaxMove" positions
    t : double (cppname = Threshold, default = 0.0001); // block would not be moved unless prediction is greater than threshold

    wm : TWeightModifier (cppname = WeightMod);

    priemka : bool;

    factors_ns : i32; // override for SaaS namespace with factors
    blend_unknown_blocks : bool;
};

struct TBlender {
    f : string (cppname = FormulaName);
    p : TBlenderParams (cppname = Params);
    Debug : bool;
    debug_factors : bool (cppname = DebugFactors, default = false);
};

struct TBlock {
    id : string (required); // block name
    shuffle : ui32 (default = 0); // if == 1 then block can be shuffled

    struct TIfUnknown {
        min_pos : ui32 (required, default = 0);
    };
    if_unknown : TIfUnknown;
};

struct TAppMetrika {
    device_id : string;
    uuid : string;
};

struct TAuth {
    login : string;
    uid   : string;
};

struct TGeo {
    lat      : string; // double inside
    lon      : string; // double inside
    region   : string; // integer inside
    accuracy : string;
    recency  : string;
};

struct TBigB {
    id : string;
    name: string;
    value: string;
    weight : string;
};

struct TBlenderRequest {
    app_metrika : TAppMetrika;
    auth        : TAuth;
    yandexuid   : string;
    icookie     : string; // yandexuid replacement

    client_id : string (required);
    locale    : string (default = "ru");
    timezone  : string (default = "Europe/Moscow");
    time_now  : ui64;

    geo : TGeo;

    blocks    : [TBlock];
    verticals : [TBlock];

    bigb : [ TBigB ];

    rearr : any;
};
