namespace NResolverConfig;

struct TServerConfig {
    port : ui16;
    threads : ui16(default = 1); // for resolve
    template_path : string; // in
    backends_path : string; // out
    log_path : string(default = "");
    resolve_interval: any (default = "10m");
    yp: {string -> string};
};
