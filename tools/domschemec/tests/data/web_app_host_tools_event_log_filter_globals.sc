namespace NEvLogFilterGlobals;


struct TCommon {
    light_codec : string (default = "lz4");
    heavy_codec : string (default = "brotli_5");
    erasure_codec : string (default = "lrc_12_2_2", allowed = ["lrc_12_2_2", "reed_solomon_6_3", "none"]);
};


struct TPreset: TCommon {
    short_ttl  : duration;
    medium_ttl : duration;
    long_ttl   : duration;
    update_interval : duration;
    auto_merge_mode : string (allowed = ["disabled", "relaxed", "economy", "manual"]);
};


struct TPer1d: TPreset {
    short_ttl  : duration (default = "3d");
    medium_ttl : duration (default = "7d");
    long_ttl   : duration (default = "30d");
    update_interval : duration (default = "10m");
    auto_merge_mode : string (default = "economy");
};


struct TPer1h: TPreset {
    short_ttl  : duration (default = "2d");
    medium_ttl : duration (default = "2d");
    long_ttl   : duration (default = "2d");
    update_interval : duration (default = "1m");
    auto_merge_mode : string (default = "relaxed");
};


struct TTest: TPreset {
    light_codec : string (default = "none");
    heavy_codec : string (default = "none");
    erasure_codec : string (default = "none");
    short_ttl  : duration (default = "600s");
    medium_ttl : duration (default = "600s");
    long_ttl   : duration (default = "600s");
    update_interval : duration (default = "5s");
    auto_merge_mode : string (default = "disabled");
};
