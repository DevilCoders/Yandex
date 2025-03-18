namespace NUgcWizardServiceConfig;

struct TConfig {
    struct TClient {
        Prefix (required) : ui32;
        LogBrokerType : string (default = "entity-ugc-log");
        Params (required) : [
            struct (array) {
                Name (required) : string;
                Options (required) : struct {
                    klevel (cppname = KeyLevel) : ui8 (default = 0);
                    save (cppname = Save) : bool (default = false);
                    max_size (cppname = MaxLength) : ui32;
                    fail_if_too_long (cppname = FailIfTooLong) : bool (default = false);
                };
            }
        ];
    };
    Clients : { string -> TClient };
    LogBroker : string;
};
