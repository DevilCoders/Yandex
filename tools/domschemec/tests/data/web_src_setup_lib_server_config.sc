namespace NSrcSetupConfig;

struct TDynamicParam {
    src(required) : [string];
    dst(required) : [string];
    prefix : string;
    is_required : bool (default = false);
    allow_empty_value : bool (default = false);
};

struct TStaticHeader {
    name(required) : string;
    value(required) : string;
};

struct TJsonParam {
    name(required) : string;
    value(required) : any;
};

struct TDynamicPostParam {
    src(required) : [string];
    dst(required) : string;
    is_required : bool (default = false);
};

//copied-and-pasted from app_host config
//TODO: remove this when .sc reuse becomes implemented in domscheme
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
    alpha : double(default = 0.01);
    time_before_rescale_ms : ui32(default = 60000);
    failed_request_coefficient : double(default = 5);
    default_response_time_ms : ui32(default = 100);
};

struct TProxyConfig {
    name(required) : string;
    addrs : [string];
    backend_descrs : [TBackEndDescr];
    hash_parts(cppname = HashParts) : [string];
    balancing_scheme : string (default = "urandom");
    timeout(required) : ui64;
    reask_attempts : ui32 (default = 5);
    task_limit : ui32 (default = 2000);
    static_params : string;
    dynamic_params : [TDynamicParam];
    static_headers : [TStaticHeader];
    dynamic_headers : [TDynamicParam];
    method : string (default = "POST");
    json_params : [TJsonParam];
    additional_path_parts : [[string]];
    static_post_params : any;
    dynamic_post_params : [TDynamicPostParam];
    post_data : [string];
    post_types : [string];
    raw_post_type : string;
    prepare_post_request_partially : bool (default = true);
    post_setup_name : string;
    pre_setup_name : string;
    allowed_http_codes : [ui16];
    fail_http_codes_without_reasks : [ui16];
    response_format : string;
    response_data_as_is : bool (default = false);
    response_type : string;
    codecs : [string];
    post_data_type : string (default = "base64");
    allow_repeated_requests : bool (default = false);
    allow_empty_response : bool (default = false);
};

struct TSetupConfig {
    name(required) : string;
    params(required) : [TJsonParam];
};

struct TConfig {
    thread_count(required) : ui16;
    proxy_thread_count : ui16;
    log(required) : string;
    setup_configs: [TSetupConfig];
    proxy_configs : [TProxyConfig];
    protocol_options : {string -> string};
};
