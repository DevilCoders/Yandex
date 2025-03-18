namespace NMstand;

struct TSqueezeParams {
    struct TFilter {
        Name : string (required, != "");
        Value : string (required, != "");
    };

    struct TExperimentForSqueeze {
        Day : string (required, != "");
        Filters : [TFilter] (required);
        FilterHash : string (required);
        Service : string (required, != "");
        TempTablePath : string (required, != "");
        Testid : string (required, != "");
        TableIndex : ui32 (required);
        IsHistoryMode : bool (required);
    };

    struct TYtParams {
        AddAcl : bool (required);
        TentativeEnable : bool (required);
        SourcePaths : [string] (required);
        YtFiles : [string] (required);
        YuidPaths : [string] (required);
        LowerKey : string (required);
        Pool : string (required);
        Server : string (required, != "");
        TransactionId : string (required);
        UpperKey : string (required);
        DataSizePerJob : ui64 (required);
        MaxDataSizePerJob : ui64 (required);
        MemoryLimit : ui64 (required);
    };

    YtParams : TYtParams (required);
    Experiments : [TExperimentForSqueeze] (required);
    RawUid2Filter: {string -> [TFilter]};
};
