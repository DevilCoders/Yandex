namespace NConfig;

struct TConfig {
    struct TSource {
        Host        : string (required);
        Timeout     : duration (default = "150ms");
        RetryPeriod : duration (default = "50ms");
        MaxAttempts : ui16 (default = 1);
    };

    struct TSources {
        CalendarHolidays : TSource (required);
        FreshRtmr        : TSource;
        FactorsKvSaaS    : TSource (required);
    };

    HttpPort        : ui16 (required);
    HttpWorkers     : ui32 (default = 100);
    HttpConnections : ui32 (default = 1000);

    Sources : TSources;
};
