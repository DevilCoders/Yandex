namespace NBlender::NProto;

struct TFeatureInfo {
    AvailibleValues : {string -> any};
};

struct TIntentMetaInfo {
    DocsCount : i32;
    FeatureInfo : {string -> TFeatureInfo};
    // viewtype -> placement -> (* or array of positions)
    AvailableCombinations : {string -> {string -> any}};
};
