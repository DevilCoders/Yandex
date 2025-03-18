namespace NUgc::NData;

struct TUgcMessage {
    type (cppname = Type, required)       : string;
    version (cppname = Version, required) : string;
};

struct TUpdateRequest {
    struct TSetOp {
        table (cppname = Table, required) : string;
        key (cppname = Key, required)     : string;
        name (cppname = Name, required)   : string;
        param (cppname = Param)           : string;
        value (cppname = Value)           : string;

        maxSize (cppname = MaxSize)       : ui64;
        options (cppname = Options)       : [string];
    };

    struct TParams {
        userId (cppname = UserId)         : string;
        visitorId (cppname = VisitorId)   : string;
        deviceId (cppname = DeviceId)     : string;
        contextId (cppname = ContextId)   : string;
        appId (cppname = AppId)           : string;
        encoded (cppname = Encoded)       : string;
        set (cppname = SetOps, required)  : [TSetOp];
    };

    id (cppname = Id, required)  : string;
    values (cppname = Values) : {string -> string};
    params (cppname = Params, required) : TParams;
};

struct TUpdateRequestMessage : TUgcMessage {
    updates (cppname = Updates, required) : [TUpdateRequest];
};
