namespace NQueryData;

struct TDocItems {
    DocIds : {string -> ui32};
    Categs : {string -> ui32};
    Urls : {string -> string};
    SnipDocIds : {string -> ui32};
    SnipCategs : {string -> ui32};
    SnipUrls : {string -> string};
};

struct TQueryDataRequest {
    ReqId : string;
    IgnoreText : bool;
    UserQuery : string;
    StrongNorm : string;
    DoppelNorm : string;
    DoppelNormW : string;

    UserRegionsIpReg : [string];
    UserRegionsIpRegDebug : [string];
    UserRegions : [string];
    UserIpMobileOp : string;
    UserIpMobileOpDebug : string;
    UserId : string;
    UserLogin : string;
    UserLoginHash : string;

    YandexTLD : string;
    UILang : string;
    SerpType : string;

    StructKeys : {string -> any};

    DocItems : TDocItems;

    FilterNamespaces : {string -> ui32};
    FilterFiles : {string -> ui32};

    SkipNamespaces : {string -> ui32};
    SkipFiles : {string -> ui32};

    Other : {string -> any};
};
