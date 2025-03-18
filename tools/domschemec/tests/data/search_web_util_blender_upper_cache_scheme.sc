// Cannot include files. <rtmapreduce/mrtasks/blender_cache/protos/scheme.sc> should be kept in mind while reading this

namespace NBlenderCache;

struct TCacheRequest {
    Key (required) : string;
    User : string;
};

struct TCacherState {
    Stage : ui8;
    Errors : string[];
    CachedRecords : {string -> any};
    HaveChanges : bool;
    // In fact, CachingRequest is TCachingRequest, which is defined in another file.
    CachingRequest : any;
    ProbingEnabled : bool (default = true);
    PushingEnabled : bool (default = true);
};
