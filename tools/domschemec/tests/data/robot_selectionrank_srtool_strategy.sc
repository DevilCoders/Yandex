namespace NStrategy;

struct TStrategy {
    log_counter_type (required) : string;
    time_interval : ui64 (default = 7);
    country (required) : string;
    label (required) : string;
    count (required) : ui64;
    method : string (default = "lazy", allowed = ["yarik", "logyarik", "superyarik", "uniform", "lazy", "loglazy"]);
};

struct TPool {
    strategies : [TStrategy];
    since : string; // actually used by python graph starter and passed as command-line argument
    allow_minuses_with_userdata (cppname = AllowMinusesWithUserData) : bool (default = true);
};

struct TPoolsSet {
    pools : [TPool];
};
