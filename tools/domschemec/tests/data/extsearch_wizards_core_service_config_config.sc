namespace NWizardServiceConfig;

struct TConfig {
    struct TSource {
        Hosts (required) : string;
        Options          : string (default = "EnableIpV6=1");
        Timeout          : duration (default = "150ms");
        CacheEnable      : bool (default = false);
        CacheFastTimeout : duration (default = "1m");
        CacheSlowTimeout : duration (default = "5m");
    };

    struct TService {
        struct TServer {
            Host (required) : string;
            Port (required) : ui16;
            Threads         : ui16;
            MaxConnections  : ui16;
            MaxQueueSize    : ui16;
            KeepAlive       : bool;
            ClientTimeout   : duration;
        };
        struct TAppHost {
            Port (required) : ui16;
            Threads         : ui16;
        };
        struct TUniLog {
            CriticalError : bool (default = true);
            ParseResponseError : bool (default = true);
        };

        LogLevel     : ui8 (default = 8);
        LogType      : string;
        EventLog     : string (default = "eventlog");
        Server       : TServer;
        AppHost      : TAppHost;
        UniLog       : TUniLog;
        NehRequester : any;
    };

    Service (required) : TService;
    Sources            : { string -> TSource };
};
