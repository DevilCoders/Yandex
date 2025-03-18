namespace NAppHost::NCommon;

struct TAppHostParams {
    reqid(required): string;
};

struct THttpRequest {
    content(required): string;
    headers(required): [[string]];
    uri(required): string;
};

struct THttpResponse {
    content(required): string;
    headers(required): [[string]];
    status_code(required): i32;
};

struct TLogAccess {
};

struct TLogProfile {
    data(required): string;
};

struct TAppHostMetaData {
    duration: ui64;
    src_errors: {string -> string};
    events: [any];
    address: string;
};
