namespace NApplyBlender;

struct TWinLoss {
    Enabled (required) : bool;
    WinFml (required) : string;
    LossFml (required) : string;
    Delta (required) : double;
    WebFeats (required) : ui32;
    BlenderFeats (required) : ui32;
    Distance (required) : ui32;
};

struct TPositionalRandom {
    struct TParams {
        PositionProbabilities : double[];
        ByPositionProbabilities : any; // array of arrays
    };
    Intents (required) : {string -> TParams};
};

struct TIntentCacheSettings {
    Duration : i32;
    ProbingEnabled : bool;
    PushingEnabled : bool;
    ForcePush : bool;
    AddToKeyExpr : string;
    MayBeUsedInInterUserMode : bool (default = true);
};

struct TIntentCache {
    Enabled : bool (default = false);
    Defaults : TIntentCacheSettings;
    ByIntent : {string -> TIntentCacheSettings};
    CustomCacheKeyPrefix : string;
    RequestInCacheKey : bool (default = true);
    UIDInCacheKey : bool (default = true);
    RegionInCacheKey : bool (default = true);
    PlatformInCacheKey : bool (default = false);
    OSFamilyInCacheKey : bool (default = false);
    GrannyInCacheKey : bool (default = false);
    GsMopInCacheKey : bool (default = false);
    InterUserMode : bool (default = false);
};

struct TApplyBlender {
    Salt : ui32;
    WinLossRearrange : TWinLoss;
    PositionalRandom : TPositionalRandom;
    IntentCache : TIntentCache;
};

struct TIntentCacheEntry {
    Show : bool;
    Pos : i32;
    IW : double;
    Place : string;
    VT (cppname = ViewType) : string;
};
