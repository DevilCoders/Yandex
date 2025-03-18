namespace NQuerySearch;

struct TQSDocsAggrOpts {
    struct TLimits {
        MaxGroups : ui32 (default = 0);
        MaxDocsInGroup : ui32 (default = 0);
    };

    BansLimits : TLimits;
    SnipsLimits : TLimits;
    VitalSnipsLimits : TLimits;
};

struct TQSSourceOpts {
    Disable : bool;
    SkipResult : bool (default = false);
    Aggregator : string (required);
    Protocol : string (required);
    ProtocolOptions : any;
};

struct TRequestRuleOptions {
    SearchMode : string (required);

    Docs : struct {
        Aggregators : {string -> TQSDocsAggrOpts};
        Sources : {string -> TQSSourceOpts};
    };

    Query : struct {
        Aggregators : {string -> any};
        Sources : {string -> TQSSourceOpts};
    };
};
