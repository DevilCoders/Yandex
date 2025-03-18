namespace NReqBundleScheme;

struct TFacetScheme {
    Expansion : string;
    RegionId: string;
};

struct TRestrictScheme {
    Facet: TFacetScheme;
    MaxBlocks: ui64;
    MaxRequests: ui64;
    Enabled: bool;

    Type: string; // use Facet; for bwd compatibility only
    MaxRequestSchemas: ui64; // use MaxRequests; for bwd compatibility only
};

struct TValueRemapScheme {
    Facet: TFacetScheme;

    // exactly one of the following
    // fields should be defined

    // = Scale * (x + Offset) / (x + Center + 2 * Offset)
    RenormFrac: struct {
        Scale (required): double;
        Center (required): double;
        Offset (required): double;
    };

    // = Value
    ConstVal : struct {
        Value (required): double;
    };

    // = floor(N * x + 0.5) / N
    Quantize: struct {
        N (required): ui64;
    };

    // = Min(MaxValue, Max(MinValue, x))
    Clip : struct {
        MinValue (required): double;
        MaxValue (required): double;
    };
};
