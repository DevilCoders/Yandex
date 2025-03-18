namespace NEvLogFilterCfg;


struct TYtCommand {
    options: struct {
        spec: {string -> any};
    };
};


struct TObservable {
    struct TObserver {
        enabled: bool;
        update_interval: duration;
    };
    struct TProgressTracker: TObserver {
    };
    struct TSafeCompleter: TObserver {
        time_limit: duration (required);
    };
    struct TStatsPusher: TObserver {
        enabled: bool (default = true);
    };
    observers: struct {
        progress_tracker: TProgressTracker;
        safe_completer: TSafeCompleter;
        stats_pusher: TStatsPusher;
    };
};


struct TSortCommand: TYtCommand, TObservable {
    spec: struct {
        partition_count: ui64;
        partition_data_size: ui64;
        partition_job_count: ui64;
        data_size_per_partition_job: ui64;
    };
};


struct TMergeCommand: TYtCommand, TObservable {
    spec: struct {
    };
};


struct TBatchCommand {
    options: struct {
    };
};


struct TComposite {
    subcommands: struct {
        sort: TSortCommand;
    };
};


struct TMapCommand: TYtCommand, TComposite, TObservable {
    type: string (required, allowed = ["map", "sort+ordered_map"]);
    spec: struct {
        job_count: ui32;
        data_size_per_job: ui64;
        mapper: struct {
            memory_limit: i64;
        };
    };
};


struct TReduceCommand: TYtCommand, TComposite, TObservable {
    type: string (required, allowed = ["sort+reduce", "map_reduce"]);
    spec: struct {
        job_count: ui64;
        data_size_per_job: ui64;
        partition_count: ui64;
        partition_data_size: ui64;
        reducer: struct {
            memory_limit: i64;
        };
    };
};


struct TTable {
    ttl: any;
    sorted: bool;
    optimize_for: string (allowed = ["scan", "lookup"]);
    compression_codec: string;
    erasure_codec: string;
    suppress_nightly_merge: bool;
    force_nightly_compress: bool;
    nightly_compression_settings: struct {
        enabled: bool;
        compression_codec: string;
        erasure_codec: string;
        min_table_size: ui64;
        min_table_age: duration;
    };
};


struct TTempTableDefaults: TTable {
    ttl: any (default = "${short_ttl}");
    // optimize_for: string (default = "scan");
    suppress_nightly_merge: bool (default = true);
    force_nightly_compress: bool (default = false);
    nightly_compression_settings: struct {
        enabled: bool (default = false);
    };
};


struct TSortedTableDefaults: TTable {
    sorted: bool (default = true);
    optimize_for: string (default = "lookup");
};


struct TSmallTableDefaults: TTable {
    ttl: any (default = "${long_ttl}");
    nightly_compression_settings: struct {
        enabled: bool (default = false);
    };
};


struct TLargeTableDefaults: TTable {
    ttl: any (default = "${short_ttl}");
    compression_codec: string (default = "${heavy_codec}");
    erasure_codec: string (default = "none");
    nightly_compression_settings: struct {
        enabled: bool (default = false);
        compression_codec: string (default = "${heavy_codec}");
        erasure_codec: string (default = "${erasure_codec}");
    };
};


struct TAutoMergeDefaults {
    mode: string (default = "${auto_merge_mode}");
    max_intermediate_chunk_count: ui64;
};


struct TDecode: TMapCommand {
    type: string (default = "map", allowed = ["map"]);
    options: struct {
        spec: struct {
            auto_merge: TAutoMergeDefaults;
        };
    };

    struct TDecodedEvents: TTempTableDefaults {
    };
    struct TGraphs: TTempTableDefaults {
    };
    struct TFullHttpEvents: TTable {
        ttl: any (default = "${short_ttl}");
        compression_codec: string (default = "${heavy_codec}");
    };
    output_tables: struct {
        temp: TDecodedEvents;
        graphs: TGraphs;
        full-http-ANY-events: TFullHttpEvents;
    };

    decode_data: bool (default = false);
};


struct TUnifyGraphs: TReduceCommand {
    type: string (default = "map_reduce");

    struct TGraphs: TTempTableDefaults {
    };
    output_tables: struct {
        graphs: TGraphs;
    };
};


struct TPreFilter: TReduceCommand {
    type: string (default = "sort+reduce");
    options: struct {
        spec: struct {
            auto_merge: TAutoMergeDefaults;
        };
    };

    struct TPreparedEvents: TSortedTableDefaults {
        ttl: any (default = "${short_ttl}");
        compression_codec: string (default = "${heavy_codec}");
        keep_data: bool (default = false);
    };
    struct TInputDumps_Error: TTempTableDefaults {
    };
    struct TInputDumps_Success: TTempTableDefaults {
    };
    struct TSourceLogRecords: TTempTableDefaults {
    };
    struct TSourceRequests: TTempTableDefaults {
    };
    struct TSourceResponses: TTempTableDefaults {
    };
    struct TOtherEvents: TTempTableDefaults {
    };
    struct TSourceErrors: TTempTableDefaults {
    };
    struct TSourceEventCount: TTempTableDefaults {
    };
    struct TServerOnlyQuant: TTempTableDefaults {
    };
    struct TSourceResponseQuant_Success: TTempTableDefaults {
    };
    struct TSourceResponseQuant_Error: TTempTableDefaults {
    };
    struct TSourceResponseTop: TTempTableDefaults {
        limit_per_job: ui32 (default = 100);
    };

    output_tables: struct {
        temp: TPreparedEvents;
        input-dumps_error: TInputDumps_Error;
        input-dumps_success: TInputDumps_Success;
        source-requests: TSourceRequests;
        source-responses: TSourceResponses;
        source-log-records: TSourceLogRecords;
        other-events: TOtherEvents;
        source-errors: TSourceErrors;
        source-ANY-event-count: TSourceEventCount;
        server-only-quant: TServerOnlyQuant;
        source-success-quant: TSourceResponseQuant_Success;
        source-error-quant: TSourceResponseQuant_Error;
        source-response-top: TSourceResponseTop;
    };

    max_frame_duration: duration (default = "60s");
    keep_temp_table: bool (default = true);
    decode_data: bool (default = false);  // TODO: implement
};


struct TFilter: TMapCommand {
    type: string (default = "sort+ordered_map", allowed = ["sort+ordered_map"]);
    output_tables: TSortedTableDefaults;

    decode_data: bool (default = false);
};


struct TFilterInputDumps_Error: TFilter {
    struct TInputDumps_Error: TLargeTableDefaults, TSortedTableDefaults {
        ttl: any (default = "${medium_ttl}");
    };
    output_tables: TInputDumps_Error;

    decode_data: bool (default = true);
};


struct TFilterInputDumps_Success: TFilter {
    struct TInputDumps_Success: TLargeTableDefaults, TSortedTableDefaults {
    };
    output_tables: TInputDumps_Success;

    decode_data: bool (default = true);
};


struct TFilterSourceLogRecords: TFilter {
    struct TSourceLogRecords: TSortedTableDefaults {
        ttl: any (default = "${short_ttl}");
        compression_codec: string (default = "${heavy_codec}");
    };
    output_tables: TSourceLogRecords;
};


struct TFilterSourceRequests: TFilter {
    struct TSourceRequests: TLargeTableDefaults, TSortedTableDefaults {
    };
    output_tables: TSourceRequests;

    decode_data: bool (default = true);
};


struct TFilterSourceResponses: TFilter {
    struct TSourceResponses: TLargeTableDefaults, TSortedTableDefaults {
    };
    output_tables: TSourceResponses;

    decode_data: bool (default = true);
};


struct TFilterOtherEvents: TFilter {
    struct TOtherEvents: TLargeTableDefaults, TSortedTableDefaults {
    };
    output_tables: TOtherEvents;
};


struct TFilterSourceErrors: TFilter {
    struct TSourceErrors: TSortedTableDefaults {
        ttl: any (default = "${medium_ttl}");
    };
    output_tables: TSourceErrors;
};


struct TCount: TReduceCommand {
    output_tables: TSmallTableDefaults;
};


struct TCountSourceEvents: TCount {
    type: string (default = "sort+reduce");

    struct TSourceEvents: TSmallTableDefaults, TSortedTableDefaults {};
    output_tables: TSourceEvents;
};


struct TQuantize: TReduceCommand {
    output_tables: TSmallTableDefaults;

    quantiles: [double] (default = [0.25, 0.5, 0.75]);
};


struct TQuantizeServerOnlyTime: TQuantize {
    type: string (default = "sort+reduce");

    struct TServerOnlyTimeQuant: TSmallTableDefaults, TSortedTableDefaults {
    };
    output_tables: TServerOnlyTimeQuant;

    quantiles: [double] (default = [0.001, 0.005, 0.01, 0.05, 0.1, 0.25, 0.5, 0.75, 0.9, 0.95, 0.99, 0.995, 0.999]);
};


struct TQuantizeSourceResponseTime_Error: TQuantize {
    type: string (default = "sort+reduce");

    struct TSourceResponseTimeQuant_Error: TSmallTableDefaults, TSortedTableDefaults {
    };
    output_tables: TSourceResponseTimeQuant_Error;

    quantiles: [double] (default = [0.5, 0.8, 0.95, 0.99, 0.999]);
};


struct TQuantizeSourceResponseTime_Success: TQuantize {
    type: string (default = "sort+reduce");

    struct TSourceResponseTimeQuant_Success: TSmallTableDefaults, TSortedTableDefaults {
    };
    output_tables: TSourceResponseTimeQuant_Success;

    quantiles: [double] (default = [0.5, 0.8, 0.95, 0.99, 0.999]);
};


struct TPartialSort: TReduceCommand {
    output_tables: TSmallTableDefaults;

    limit: ui32 (default = 100);
};


struct TPartialSortSourceResponseTime: TPartialSort {
    type: string (default = "sort+reduce");

    struct TSourceResponseTimeTop: TSmallTableDefaults, TSortedTableDefaults {
    };
    output_tables: TSourceResponseTimeTop;
};


struct TConfig {
    commands: struct {
        all: struct {
        };
        decode: TDecode;
        unify-graphs: TUnifyGraphs;
        pre-filter: TPreFilter;
        filter: struct {
            input-dumps_error: TFilterInputDumps_Error;
            input-dumps_success: TFilterInputDumps_Success;
            source-log-records: TFilterSourceLogRecords;
            source-requests: TFilterSourceRequests;
            source-responses: TFilterSourceResponses;
            other-events: TFilterOtherEvents;
            source-errors: TFilterSourceErrors;
        };
        count: struct {
            source-ANY-event-count: TCountSourceEvents;
        };
        quantize: struct {
            server-only-quant: TQuantizeServerOnlyTime;
            source-error-quant: TQuantizeSourceResponseTime_Error;
            source-success-quant: TQuantizeSourceResponseTime_Success;
        };
        partial-sort: struct {
            source-response-top: TPartialSortSourceResponseTime;
        };
    };
    common: struct {
        observers: struct {
            update_interval: any (default = "${update_interval}");
        };
        solomon: struct {
            common_labels: struct {
                project: string (default = "apphost");
                cluster: string (default = "hahn");
                service: string (default = "event_log_filter");
            };
        };
        nightly_compression_settings: struct {
            pool: string;
            owners: [string] (default = ["pmatsula", "sharpeye", "librarian", "manokk"]);
        };
    };
};
