namespace NDoc2Group;

struct TDocConfig {
    Enabled : bool (default = false);
    Filter : string (default = "");
    Hosts : {string -> bool};
    Locale (required) : string;
    MinDocCount : ui32 (default = 1);
    MaxDocCount : ui32 (default = 1);
    SkipNav : bool (default = false);
    Service : string (default = "");
    RemoveFromOrigGrouping : bool (default = true);
};

struct TDoc2Group {
    Enabled : bool(default = false);
    DocConfigs : {string -> {string -> TDocConfig}}; // src_grouping -> dst_grouping -> config
};
