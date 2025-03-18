namespace NJokerConfig;

struct TYdbConfig {
    DataBase : string (required);
    Endpoint : string (required);
    Token : string;
};

struct TConfig {
    HttpPort (required) : ui16;
    HttpThreads : ui16 (required, default = 10);
    EventLog : string;
    YDb : TYdbConfig;
    LocalStorage (required) : struct {
        Path (required) : string;
    };
};
