namespace NAppHostHttpAdapterConfig;

struct TLoggingConfig {
    log_request_cgi_parameters : bool (default=true);
    log_request_body_content : bool (default=false);
    log_response_body_content : bool (default=false);
};

struct TGraphSettings {
    logging_config(required) : TLoggingConfig;
    proxy_request : bool (default=false);
    proxy_response : bool (default=false);
};

struct TPathSettings {
    graph(required) : string;
    prefix : bool (default=false);
};

struct TRoutingConfig {
    path2graph(required) : {string -> string};
    paths : {string -> TPathSettings};
    graph2settings : {string -> TGraphSettings};
    default_graph : string;
};

struct TBackendConfig {
    timeout(required) : ui64;
    attempts : ui32 (default=10);
    backends(required) : [string];
    grpc_port_offset: i16(default=1);
};

struct THttpServerOptions {
    enable_compression : bool (default=false);
};

struct TLimits {
    soft_limit : ui64(default = 40000);
    hard_limit : ui64(default = 50000);
};

struct TInterval {
    left(required) : ui64;
    right(required) : ui64;
    precision(required) : ui64;
};

struct TConfig {
    log(required) : string;
    access_log(required) : string;
    access_log_from_report_proxy(required) : string;
    request_log(required) : string;
    routing_config_path : string;
    backend_config(required) : TBackendConfig;
    logging_config(required) : TLoggingConfig;
    http_server_options : THttpServerOptions;
    protocol_options : {string -> string};
    response_time_hist_intervals : [TInterval];
    response_size_hist_intervals : [TInterval];
    request_chunk_size_hist_intervals : [TInterval];
    http_input_connections_limits : TLimits;
    http_output_connections_limits : TLimits;
    port : ui16;
    client_timeout : ui32(default = 10);
    grpc_threads: ui32(default = 1);
    enable_streaming_by_default: bool (default=false);
};
