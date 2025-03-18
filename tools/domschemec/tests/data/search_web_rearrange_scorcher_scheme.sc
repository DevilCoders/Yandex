namespace NScorcher;

struct TRelevanceModifierRule {
    Mul : double (default = 1.0);
    Add : i64 (default = 0);
};

struct TCondition {
    RatioThreshold : double;
    MinNum : i32 (default = 5);
    Type : string;
};

struct TRule : TRelevanceModifierRule, TCondition {
};


struct TParameters {
    Enabled : bool (default = false);
    DryRun : bool (default = false);
    Verbosity : i32 (default = 0);

    Irrel : TRule;
    Rel : TRule;

    RPThreshold : double (default = -1.0);
    RPRule : TRelevanceModifierRule;

    Weight : struct {
        Assessor : i32 (default = 3);
        Toloker : i32 (default = 1);
    };

    ExactRegionMatch : bool (default = false);
};
