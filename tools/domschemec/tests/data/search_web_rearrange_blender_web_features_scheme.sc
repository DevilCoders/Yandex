namespace NBlenderWebFeaturesSc;

struct TMatcherConfig {
    Type (required): string;
    Params : any;
};

struct TCalcConfig {
    Intent (required) : string;
    Slice (required) : string;
    IndexRange : string;
    Disabled : bool (default = false);
    Matcher(required) : string;
    MaxCount : ui32 (default = 10);
};

struct TJoinConfig {
    Matcher(required) : string;
    Disabled : bool (default = false);
};
