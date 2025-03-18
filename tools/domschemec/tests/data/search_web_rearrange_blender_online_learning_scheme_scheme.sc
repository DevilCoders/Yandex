namespace NBlenderOnlineLearning::NSchemeInternal;

struct TSimpleRandomizePolicyParams {
    RandomizeBundle (required) : string;
    FullRandomReqCount : ui32 (default = 0);
    RandomProbA : double (default = 1.0);
    RandomProbB : double (default = 1.0);
    RandomProbFunc : string (default = "linear");
    MinRandomProb : double (default = 0.0);
    RandomSaltMinutes : ui32 (default = 30);
    FixedPosReqCount : ui32 (default = 0);
    FixedPos : ui32 (default = 0);
};

struct TTrainModelParams {
    Intent (required) : string;
    Locales : string[];
    UserInterfaces: string[];

    FeatureSources (required) : string[];
    ModelType : string (default = "linear_softmax");
    ClassNumber : ui32 (default = 11);
    FeatureCount (required) : ui32;

    ClickBoost : double (default = 1.0);
    SurplusVersion : string;
    UseLocalRandom : bool (default = false);
    BatchSize : ui32 (default = 300);
    LearningRate : double (default = 1.0);
    RegValue : double (default = 0.0);
    EvaluateWindowSize : ui32 (default = 3000);
    ModelVersion : ui32 (default = 0);
};


struct TApplyModelParams {
    Intent (required) : string;
    Locales : string[];
    UserInterfaces: string[];

    FeatureSources : string[];

    ModelSource : string (default = "rtmr");
    ModelSourceKey : string;
    ApplyCondition : string (default = "has_doc");

    ApplyPriority : double (default = 0.5);
    RandomizeParams : TSimpleRandomizePolicyParams;
    FallBackBlenderPos : ui32;
};



