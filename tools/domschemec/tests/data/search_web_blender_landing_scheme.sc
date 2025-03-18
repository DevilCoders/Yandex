namespace NBlender::NLanding;

struct TIntentInfo {
    Disabled : bool (default = false);
    ExpireTs : ui64;
    RuleName (required) : string;
    Priority : double;
};
