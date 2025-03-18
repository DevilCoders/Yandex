namespace NBlender::NProto;

struct TResolveRequest {
    struct TMain {
        mode : string (required, allowed = ["allow_all"]);
        pos : ui32 (required);
        view_type : string;
        action_prior : double;
    };
    struct TRight {
        mode : string (required, allowed = ["exclusive", "allow_all", "allow_only", "allow_except"]);
        allowed_list : {string -> bool};
        except_list : {string -> bool};
        requires : {string -> bool};
        pos : ui32 (required);
        view_type : string;
    };
    struct TWizplace {
        mode : string (required, allowed = ["allow_all", "exclusive"]);
        pos : ui32 (required);
        view_type : string;
        action_prior : double;
    };

    main : TMain;
    right : TRight;
    wizplace : TWizplace;
    utility (required) : double;
    fulfilled : bool;
};
