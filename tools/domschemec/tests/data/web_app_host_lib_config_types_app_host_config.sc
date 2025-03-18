namespace NAppHostConfig;

struct TBackEndDescr {
    addr : string;
    ip : string;
    port : ui16;
    protocol : string;
    host : string;
    path : string;
    weight : double(default = 1);
    min_weight : double(default = 0);
    max_weight : double(default = 1);
    alpha : double(default = 0.0000034);
    time_before_rescale_ms : ui32 (default = 60000);
    failed_request_coefficient : double (default = 10);
    default_response_time_ms : ui32 (default = 100);
    consecutive_fail_penalty : double (default = 80);
};

struct TAliasConfig {
    param_alias : [string];
    ask_alias : [string];
    skip_alias : [string];
    addr_alias : [string];
    stat_alias : [string];
};

struct TSourceBase {
    codecs : [string];
    alias_config: TAliasConfig;
};

struct TStaticSource : TSourceBase {
    embed(required) : any;
};

struct TBatchSource : TSourceBase {
    batch_path(required) : string;
};

struct TReliabilityLevel {
    success_ratio:          double;
    acceptance_probability: double;
};

struct TMetaBackEndDescr {
    backend_descrs(cppname = BackEndDescrs) : [TBackEndDescr];
    balancing_scheme : string(default = "urandom");
    reliability_levels : [TReliabilityLevel];
    weight : double(default = 1);
    location : string;
    test_request_count : ui32(default = 250);
    ring_point_count : ui32(default = 1000);
    history_buffer_len : ui32(default = 100);
    seed : ui64(default = 0);
    hash_parts(cppname = HashParts) : [string];
};

struct TSourceLoadControlConfig {
    requests_per_reask : ui32 (default = 0);
    decay : double;
    max_reask_budget : double (default = 100);
    optimized_response_time : ui64 (default = 0);

    // simple model
    threshold : double (default = 1.0);
    soft_timeout : any;
};

struct TSourceLoggingConfig {
    dump_request_probability : double(default = 0);
    dump_response_probability : double(default = 0);
    dump_error_requests : bool(default = false);
};

// variant: TBackEndConfig | TMetaBackEndConfig | TMetaSearchBackEndConfig
struct TBackEndConfigBase {
    grpc_port_offset: i16(default = 0);
    use_grpc: bool(default = false);
};

struct TBackEndConfig : TBackEndConfigBase, TMetaBackEndDescr {
};

struct TMetaBackEndConfig : TBackEndConfigBase {
    balancing_scheme : string(default = "inorder");
    meta_backend_descrs(cppname = MetaBackEndDescrs) : [TMetaBackEndDescr];
};

struct TMetaSearchBackEndConfig : TBackEndConfigBase {
    cgi_search_prefix(required) : string;
    options(required) : string;
};

struct TSource : TSourceBase {
    timeout(required): any;
    // variant: TBackEndConfig | TMetaBackEndConfig | TMetaSearchBackEndConfig
    backend_config(cppname = BackEndConfig): TBackEndConfigBase;
    max_resolution_attempts : ui32(default = 2);
    max_attempts : ui32(default = 2);
    max_request_error_attempts : ui32(default = 1);
    max_fast_error_attempts : ui32(default = 15);
    max_crash_error_attempts : ui32(default = 2);
    hedge_delta : any(default = "0ms");
    soft_timeout : any;
    force_request_if_empty : bool(default = false);
    request_fraction : double;
    blocking : bool(default = true);
    load_control_config : TSourceLoadControlConfig;
    output_name : string;
    transparent : bool(default = false);
    balancing_id : string;
    logging_config : TSourceLoggingConfig;
    use_grpc : bool(default = false);
    grpc_connection_timeout : duration (default = "0s");
    use_encryption: bool(default = false);
    http_proxy : bool(default = false);
    use_protobuf_prefix: bool(default = false);
    use_streaming : bool(default = false);
    important : bool(default = true);
    tvm_id : ui64;

    source: string;
    balancing_scheme: string;
    handler: string;
    required: bool(default = true);

    expects_debug_info: bool(default = false);
    accepts_answer_infos: bool(default = false);
};

struct TFlag {
    src(required) : string;
    name(required) : string;
};

struct TDepGraph {
    version : ui32(default = 3);
    quota_group : string;
    timeout : any(default = "10000ms");
    sources(required) : {string -> any};
    input_deps : [string];
    output_deps : [string];
    graph(required) : {string -> [string]};
    edge_expressions : {string -> string};
    dump_source_requests : bool(default = false);
    dump_source_responses : bool(default = false);
    dump_input : bool;  // XXX: deprecated
    dump_input_probability : double;
    alias_config : TAliasConfig;
};

struct TInterval {
    left(required) : ui64;
    right(required) : ui64;
    precision(required) : ui64;
};

struct TLimits {
    soft_limit : ui64(default = 40000);
    hard_limit : ui64(default = 50000);
};

struct TTvmConfig {
    self_id(required) : ui64;
    keys_update_interval : ui64(default = 14400);
    tickets_update_interval : ui64(default = 3600);
};

struct TNehTransportBackendOptions {
    safe_to_cancel_threshold : double;
};

struct TZkOptions {
    hosts(required) : [string];
    path(required) : string;
    prefix : string (default = "");
};

struct TKikimrOptions {
    addr(required) : string;
    table(required) : string;
    prefix : string (default = "");
    check_interval : ui64(default = 5000);
};

struct TPatcherConfig {
    patch_paths : [string];
    scan_interval : ui64(default = 1000);
    zk_options : TZkOptions;
    kikimr_options : TKikimrOptions;
};

struct TFullConfig {
    log : string(default = "");
    profile_log : string(default = "profile_log");
    enable_profile_log : bool(default = false);
    log_degrade_threshold : ui64(default = 200000000);
    conf_dir(required) : string;
    fallback_conf_dir : string;
    backends_path : string;
    fallback_backends_path: string;
    graph_name_mapping_path : string(default = "");
    bind : [string];
    port : ui16;
    threads(required) : ui16;
    grpc_threads : ui16 (default = 4);
    grpc_server_threads : ui16 (default = 2);
    grpc_ping_timeout : ui64 (default = 1000);
    grpc_connection_timeout : duration (default = "25ms");
    executor_threads(required) : ui16;
    background_threads : ui16 (default = 1);
    total_quota : ui16;
    total_quota_threshold : double (default = 0.7);
    protocol_options : {string -> string};
    http_input_connections_limits : TLimits;
    http_output_connections_limits : TLimits;
    source_codecs : [string];
    response_time_hist_intervals : [TInterval];
    response_size_hist_intervals : [TInterval];
    request_chunk_size_hist_intervals : [TInterval];
    default_load_control_config : TSourceLoadControlConfig;
    load_control_model: string (default = "standard");
    debug_info_version : ui16 (default = 0);
    use_async_stats_merge : bool (default = false);
    use_async_load_control : bool (default = false);
    track_debug_request_stats : bool (default = true);
    tvm_config : TTvmConfig;
    patcher_config : TPatcherConfig;
    max_global_timeout : any (default = "600000ms");
    allow_empty_backends(cppname = AllowEmptyBackEnds): bool(default = true);
    signal_ttl : ui64 (default = 3600000);
    average_stats_window : duration (default = "1m");
    average_stats_bucket_size : duration (default = "5s");
    strict_graph_version : bool (default = false);
    neh_transport_backend_options : TNehTransportBackendOptions;
    group_quotas : {string -> double};
    background_task_limit : ui16 (default = 500);
    dump_input_probability : double(default = 0.01);
    graph_cache_capacity: ui64(default = 10000000);
    graph_cache_ttl: ui64(default = 0);
    certificate_path: string;
    private_key_path: string (default = "./apphost.key.pem");
    root_certificates_path: string;
    golovan_output_zero_values: bool (default = false);
    golovan_cache_update_duration: ui64(default = 0);
};
