namespace NBlenderCache;

struct TCachingRequestEntry {
    Duration (required): i64;
    Value (required): any;
};

struct TCachingRequestRecord {
    Entries (required) : {string -> TCachingRequestEntry};
};

struct TCachingRequest {
    Records (required) : {string -> TCachingRequestRecord};
};

struct TCachedEntry {
    Expire (required) : i64;
    Value (required) : any;
};

struct TCachedRecord {
    Entries (required) : {string -> TCachedEntry};
};

struct TAggregatorState {
    Record (required) : TCachedRecord;
    Timestamps (required): {string -> i64};
};
