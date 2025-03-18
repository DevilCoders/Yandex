namespace NBenchmarkScheme;

struct TBackend {
    predefined-name : string (default = "");
    fail-rate : double (default = 0.05);
    fail-time : ui32 (default = 20);
    response-time : ui32 (default = 10);
};

struct TScenario {
    backends : [TBackend];
    request-count : ui32 (default = 1000000);
    requests-per-iteration : ui32 (default = 100);
    time-per-request : ui32 (default = 10);
};

struct TConfig {
    test-scheme : string (default = "weighted");
    against-scheme : string (default = "weighted-fd");
    scenario : TScenario;
};
