namespace NExtendedMx::NSCProto;

struct TAbsoluteRelevance {
    Enabled (required) : bool;
    Expr (required) : string;
    Method (required) : string;
    Top (required) : ui32;
    Owner : string;
};

struct TBundleMeta {
    RandomSeed : any;
    AbsoluteRelevance : TAbsoluteRelevance;
    FeaturesInfo : any;
    // node for storing all training related information: link to nirvana workflow, etc.
    TrainDetails : any;
};

struct TBundle {
    XtdType (required) : string;
    Alias : string;
    Meta : TBundleMeta;
};

struct TFml {
    Name (required) : string;
    Bin (required) : any;
};

struct TStump {
    Threshold (required) : double;
    Value (required) : double;
};

struct TStumps {
    Default (required) : double;
    Stumps : TStump[];
};

struct TResultTransform {

    struct TClampParams {
        Min (required) : double;
        Max (required) : double;
    };

    Distrib : string;
    Clamp : TClampParams;
    Stumps : TStumps;
    PiecewiseLinearTransform : TStumps;
};

struct TFeatureInfo {
    Name (required) : string;
    Values (required) : string[];
    Binary (required) : bool;
};


struct TClickIntParams {
    From (required) : double;
    Step (required) : double;
    Thresholds (required) : double[];
    TransformStepBeforeCalc : bool (default=false);
    UseThresholds : bool (default=true);
    ResultTransform : TResultTransform;
    DumpPredicts : bool (default = false);
};

struct TClickInt {
    Fml (required) : TFml;
    Params (required) : TClickIntParams;
};

struct TPositionalClickInt {
    struct TPositionalParams {
        Count (required) : ui32;
        AdditionalFeatures : TFeatureInfo[];
        FeatureName (required) : string;
        DefaultPos (required) : ui32;
        LogIndices : ui32[];
    };

    Fml (required) : TFml;
    Params (required) : TPositionalParams;
};

struct TClickRegTreeParams {
    struct TPositionalParams {
        FeatureName (required) : string;
        DefaultPos (required) : ui32;
    };

    Step (required) : double;
    StepsCount (required) : ui32;
    Threshold (required) : double;
    ClickRegTreeType (required) : string;
    ResultTransform : TResultTransform;
    PositionParams : TPositionalParams;
    StartIntentWeight : double;
};

struct TClickRegTree {
    Fml (required) : TFml;
    Params (required) : TClickRegTreeParams;
};

struct TClickIntRandom {
    struct TClickIntRandomParams {
        From (required) : double;
        Step (required) : double;
        Count (required) : ui32;
        ResultTransform : TResultTransform;
        AdditionalSeed : string;

        AdditionalFeatures : TFeatureInfo[];
    };

    Params (required) : TClickIntRandomParams;
};

struct TIdxRandom {
    struct TIdxRandomParams {
        Probabilities (required) : double[];
        ProbabilitiesByPos : [[double]];
        ShowProbaMultiplierIfFiltered : double;
        DoNotShowPos : ui32 (default=100);
        Idx2Pos : ui32[];
        FeatureName : string;
        AdditionalSeed : string;
        AdditionalFeatures : TFeatureInfo[];
    };

    Params (required) : TIdxRandomParams;
};

struct TThompsonSampling {
    struct TThompsonSamplingParams {
        ShowScoreThreshold : double (default = 0.5);
        UniformProba : double (default = 0);
        DistributionName : string (default = "beta");
        DoNotShowPos : ui32 (default = 100);
        AdditionalSeed : string;
        UseTotalCountersAsShows : bool (default = false);
        WarmupDuration : double (default = 0.0);
        CountersMode : string (default = "from_features");
        ShowsAddition : double (default = 0.0);
        ShowsMultiplier : double (default = 1.0);
        AdditionalFeatures : TFeatureInfo[];
    };

    WarmupRandomizeBundle : TBundle;
    Params (required) : TThompsonSamplingParams;
};

struct TMultiFeatureLocalRandom {
    struct TMultiFeatureLocalRandomParams {
        struct TFeature {
            Name (required) : string;
            DefaultValueIdx : ui32;
            Values (required) : [any];
            TransitionMatrix (required) : [[double]];
        };
        Features : [TFeature];
        NoShowWeight : double;
        AdditionalSeed : string;
    };
    Params (required) : TMultiFeatureLocalRandomParams;
};

struct TWeightRandom {
    struct TIdxRandomParams {
        Probabilities : [[double]];
        FeatureName : string;
        AdditionalSeed : string;
        Step (required) : double;
        IWTransform : TResultTransform;
        ResultTransform : TResultTransform;
    };

    Params (required) : TIdxRandomParams;
};

struct TViewPlacePosRandom {
    struct TViewPlacePosRandomParams {
        //             viewtype -> placement -> posweights
        ShowWeights : {string -> {string -> [double]}};
        NoShowWeight : double;
        AdditionalSeed : string;
    };

    Params (required) : TViewPlacePosRandomParams;
};

struct TCombination {
    ViewType : string (default="");
    Place : string (default="main");
    Pos (required) : ui32;
    Weight : double (default=0.0);
};

struct TMultiShowRandom {
    struct TMultiShowRandomParams {
        struct TShow {
            Place (required) : string;
            Pos (required) : ui32;
            ViewType : string;
        };
        struct TWeightedShows {
            Weight (required) : double;
            Shows (required) : [TShow];
        };
        ShowItems (required) : [TWeightedShows];
        NoShowWeight : double;
        AdditionalSeed : string;
    };
    Params (required) : TMultiShowRandomParams;
};

struct TFactorFilter {
    ResultTransform : TResultTransform;

    struct TParams {
        Expression (required) : string;
        RandomAdditionalSeed: string;
    };

    Params (required) : TParams;
};

struct TSimple {
    struct TPositionalParams {
        FeatureName (required) : string;
    };

    Fml (required) : TFml;
    ResultTransform : TResultTransform;
    PositionalParams : TPositionalParams;
};

struct TMultiBundleParams {
    Expression (required) : string;
    ResultTransform : TResultTransform;
    LogChildResult : bool (default=false);
};

struct TMultiBundle {
    Bundles : TBundle[];
    Params (required) : TMultiBundleParams;
};

struct TMultiBundleByFactors {
    struct TParams {
        Expression (required) : string;
    };

    Bundles : TBundle[];
    Params (required) : TParams;
};

struct TMultiBundleWithExternalViewTypeMx {
    ExternalMx (required) : TFml;
    ExternalMxClasses (required) : string[];
    RegularBundle (required) : TBundle;
};

struct TWinLossLinear {

    struct TLinearFml {
        struct TParams {
            A (required) : double;
            B (required) : double;
            ResultTransform : TResultTransform;
        };

        Fml (required) : TFml;
        Params (required) : TParams;
    };

    struct TParams {
        From (required) : double;
        Step (required) : double;
        Count (required) : ui32;
        ResultTransform : TResultTransform;

        AdditionalFeatures : TFeatureInfo[];
    };

    Win (required) : TLinearFml;
    Loss (required) : TLinearFml;
    Params (required) : TParams;
};

struct TMultiTargetMx {
    struct TParams {
        From (required) : double;
        Step (required) : double;
        Count (required) : ui32;

        BaseFml : TFml;
        BaseFmlTransform : TResultTransform;

        TargetUseStep(required) : bool;
        Targets (required) : TFml[];
        TargetsAdditionalFeatures : TFeatureInfo[];

        ResultFml (required) : TFml;
        ResultUseFeats : bool (default = false);
        ResultUseAdditionalFeats : bool;
        ResultFmlTransform: TResultTransform;

        TreatStepAsFeature : string;
        DefaultStepAsFeature : ui32 (default = 100);
        TreatLastStepAsDontShow : bool (default = false);

        Greedy : bool (default = false);

        Threshold : double (default = 0);
    };

    Params : TParams;
};

struct TCascadeMx {
    struct TParams {
        Targets (required) : TFml[];

        ResultFml (required) : TFml;
        ResultUseFeats : bool (default = false);
        ResultFmlTransform: TResultTransform;
    };

    Params : TParams;
};

struct TMxWithMetaMxs {
    struct TSubFml {
        SubFml (required) : TFml;
        FactorIndex (required) : ui32;
    };

    SubFmls : TSubFml[];
    UpperFml (required) : TFml;
    ResultTransform : TResultTransform;
};

struct TCombinationsInfo {
    struct TFeature {
        Name (required) : string;
        Values (required) : any[];
        Binary : bool (default = false);
    };

    Features (required) : TFeature[];
    AllowedCombinations (required) : [[ui32]];
    UseNoShowAsCombination : bool (default = false);
    NoShow (required) : ui32[];
};


struct TMultiTargetCombinations {
    struct TParams {
        Targets (required) : TFml[];
        ResultFml (required) : TFml;
        ResultUseFeats : bool (default = false);
        Combinations : TCombinationsInfo;
    };

    Params : TParams;
};

struct TMultiClassWithFilter {
    struct TParams {
        struct TClassBounds {
            Min : double;
            Max : double;
        };
        FilterThreshold (required) : double;
        MultiClassBounds (required) : {string -> TClassBounds};
        FeatureName (required) : string;
    };

    FilterCalcer (required) : TBundle;
    MultiClassCalcer (required) : TBundle;
    Params (required) : TParams;
};

struct TClickIntBruteForce {
    struct TParams {
        AdditionalFeatures : TFeatureInfo[];
        ResultTransform: TResultTransform;
        ZeroIfBadScore : bool (default = false);
    };

    Fml (required) : TFml;
    BaseFml : TFml;
    Params (required) : TParams;
};

struct TSurplusPredict {
    RealCalcer (required) : TBundle;
    PredictCalcer (required) : TFml;
};

struct TTrimmedFmlGroup {
    With : TFml[];
    Without: TFml[];
};

struct TMultiTargetMxFast {
    struct TParams {
        From (required) : double;
        Step (required) : double;
        Count (required) : ui32;

        BaseFml : TFml;
        BaseFmlTransform : TResultTransform;

        TargetUseStep(required) : bool;
        Targets (required) : TTrimmedFmlGroup[];
        TargetsAdditionalFeatures : TFeatureInfo[];

        ResultFml (required) : TFml;
        ResultUseFeats : bool (default = false);
        ResultUseAdditionalFeats : bool;
        ResultFmlTransform: TResultTransform;

        TreatStepAsFeature : string;
        DefaultStepAsFeature : ui32 (default = 100);
        TreatLastStepAsDontShow : bool (default = false);

        Greedy : bool (default = false);

        Threshold : double (default = 0);
    };

    Params : TParams;
};

struct TPositionalMultiClassSAMClickInt {
    struct TParams {
        FeatureName (required) : string;
        DefaultPos (required) : ui32;
    };

    Fml (required) : TFml;
    Params (required) : TParams;
};

struct TPositionalSoftmaxWithSubtargets {
    struct TParams {
        FeatureName (required) : string;
        DefaultPos (required) : ui32;
        UseSourceFeaturesInUpperFml : bool (default = false);
    };

    Upper (required) : TFml;
    Subtargets (required) : TFml[];
    Params (required) : TParams;
};

struct TPositionalSoftmaxWithPositionalSubtargets {
    struct TParams {
        FeatureName (required) : string;
        DefaultPos (required) : ui32;
        PositionCount (required) : ui32;
        UseSourceFeaturesInUpperFml : bool (default = false);
    };

    Upper (required) : TFml;
    Subtargets (required) : TFml[];
    Params (required) : TParams;
};

struct TFeature {
    Name (required) : string;
    Values (required) : any[];
};

struct TMultiFeatureSoftmax {
    struct TParams {
        Features (required) : TFeature[];
        AllowedCombinations (required) : [[ui32]];
        NoPosition (required) : any[];
        SetFeaturesAsInteger (required) : bool;
        SetFeaturesAsBinary (required) : bool;
        UseSourceFeaturesInUpperFml (required) : bool;
        FilterThreshold : double (default = 0.0);
        ShowScoreThreshold : double (default = 0.0);
        TopPosInsteadOfMax : bool (default = false);
    };

    ForceUnimodal : bool (default = false);
    Upper (required) : TFml;
    Subtargets (required) : TFml[];
    Filter : TFml;
    Params (required) : TParams;
};

struct TMultiFeatureWinLossMultiClass {
    struct TFeature {
        Name (required) : string;
        Values (required) : any[];
    };

    struct TCombinationBoost {
        Combination (required) : ui32[];
        SurplusAddition : double (default = 0.0);
        ClickMultiplier : double (default = 1.0);
    };

    struct TParams {
        Features (required) : TFeature[];
        AllowedCombinations (required) : [[ui32]];
        NoPosition (required) : any[];
        NoPositionAlwaysAvailable : bool (default = false);
        SetFeaturesAsInteger (required) : bool;
        SetFeaturesAsBinary (required) : bool;
        SetFeaturesAsCategorical : bool (default = false);
        UseSourceFeaturesInUpperFml (required) : bool;
        ShowScoreThreshold : double (default = 0.0);
        ClickBoost : double (default = 1.0);
        BoostsByCombination : TCombinationBoost[];
        LossCategShift : i32;
        IsMultiShow : bool (default = false);
        PickMaxWinPosition : bool (default = false);
        UseUnshiftedSurplusForScoreThreshold : bool (default = false);
        WinSubtargetIndex : ui32 (default = 0);
    };

    struct TOrganicPosDistr {
        WizPos : ui32;
        Feats : {string -> any};
        Probs (required) : double[];
        RightProb : double (default = 0.0);
        WizplaceProb : double (default = 0.0);
    };

    struct TViewTypeShift {
        Mult : double (default = 1.0);
        Add : double (default = 0.0);
    };

   struct TRelevBoostParams {
        SurplusCoef : double (default = 1.0);
        RelevDCGCoef : double (default = 0.0);
        WizRelevCalcer (required) : TFml;
        WizRelevTransform : TResultTransform;
        DocRelevsIndexes (required) : ui32[];
        DocRelevTransform : TResultTransform;
        WinSubtargetIndex : ui32;  // deprecated
        DCGDocCount : ui32 (default = 5);
        OrganicPosProbs : TOrganicPosDistr[];
        ViewTypeShift : {string -> TViewTypeShift};
        UseYandexTierRelevance : bool (default = false);
    };

    struct TAdditionModelParams {
        Model (required) : TFml;
        AdditionScoreCoef (required) : double;
        RemainNoShowPositionThreshold : ui32;
        RemainPositionIfShow : bool (default = false);
        SubtractNoPositionScore : bool (default = false);
        Features (required) : TFeature[];
        AllowedCombinations (required) : [[ui32]];
        SetFeaturesAsInteger (required) : bool;
        SetFeaturesAsBinary (required) : bool;
        SetFeaturesAsCategorical : bool (default = false);
    };

    struct TPositionFilterParams {
        Model (required) : TFml;
        Threshold (required) : double;
        Features (required) : TFeature[];
        AllowedCombinations (required) : [[ui32]];
        SetFeaturesAsInteger (required) : bool;
        SetFeaturesAsBinary (required) : bool;
        SetFeaturesAsCategorical : bool (default = false);
    };

    struct TRecalcParams {
        struct TParams {
            ShowScoreThreshold : double;
            ClickBoost : double;
            BoostsByCombination : TCombinationBoost[];
            PickMaxWinPosition : bool;
            UseUnshiftedSurplusForScoreThreshold : bool;
        };
        struct TRelevBoostParams {
            SurplusCoef : double;
            RelevDCGCoef : double;
        };
        struct TAdditionModelParams {
            AdditionScoreCoef : double;
            RemainNoShowPositionThreshold : ui32;
            RemainPositionIfShow : bool;
        };
        Params : TParams;
        RelevBoostParams : TRelevBoostParams;
        AdditionModelParams : TAdditionModelParams;
    };

    Upper (required) : TFml;
    Subtargets (required) : TFml[];
    Params (required) : TParams;
    RelevBoostParams : TRelevBoostParams;
    AdditionModelParams : TAdditionModelParams;
    PositionFilterParams : TPositionFilterParams;
};


struct TNeuralNetInputInfo {
    Name (required) : string;
    BeginPos (required) : ui32;
    EndPos (required) : ui32;
};

struct TNeuralNet {

    Model (required) : TFml;
    NumFeats (required): ui32;
    InputsInfo (required) : [TNeuralNetInputInfo];
    VariablesInfo (required) : [TNeuralNetInputInfo];
    Output (required) : string;
};

struct TPositionalPerceptron {
    struct TParams {
        FeatureName (required) : string;
        Positions (required) : i32[];
    };

    Perceptron : string;
    Subtargets (required) : TFml[];
    Params (required) : TParams;
};

struct TPositionalMajorityVote {
    struct TParams {
        FeatureName (required) : string;
        Positions (required) : i32[];
    };

    struct TBorderVotes {
        Answers (required) : double[];
    };

    struct TFeatureVotes {
        Borders (required) : double[];
        BorderVotes (required) : TBorderVotes[];
    };

    Voters (required) : TFeatureVotes[];
    Subtargets (required) : TFml[];
    Params (required) : TParams;
};

struct TPositionalBinaryComposition {
    struct TPositionPredictor {
        Position (required) : ui32;
        Bias : double (default = 0.0);
        Multiplier : double (default = 1.0);
        Calcer (required) : TBundle;
        DoSigmoid : bool (default = false);
    };
    struct TParams {
        LogPositionScore : bool (default = false);
        PosFeatureName (required) : string;
        NotShownPosition : ui32 (default = 100);
        NotShownScore: double (default = 0.0);
    };
    Params (required) : TParams;
    Predictors (required) : TPositionPredictor[];
};

struct TOnePositionBinary {
    struct TParams {
        Threshold : double (default = 0.0);
        Position : ui32 (default = 0);
        PositionFeatureName (required) : string;
        NotShownPosition : ui32;
        AdditionalFeaturesToSet : {string -> string};
    };
    BinaryClassifier (required) : TBundle;
    Params (required) : TParams;
};

struct TBinaryWithArbitraryResult {
    struct TParams {
        Threshold : double (default = 0.0);
        FeaturesToSetOnPositive : {string -> any};
        FeaturesToSetOnNegative : {string -> any};
    };
    BinaryClassifier (required) : TBundle;
    Params (required) : TParams;
};

struct TAnyWithRandom {
    struct TParams {
        RandomProbability (required) : double;
        AdditionalSeed : string;
    };
    RandomizeBundle (required) : TBundle;
    ProdBundle (required): TBundle;
    Params (required) : TParams;
    struct TRecalcParams {
        ProdBundleParams : any;
    };
};

struct TAnyWithFilter {
    struct TParams {
        Threshold (required) : double;
        DefaultPos (required) : ui32;
        DefaultValue (required) : double;
        FilterIfLess : bool (default = true);
    };
    FilterModel (required) : TFml;
    ProdBundle (required): TBundle;
    Params (required) : TParams;
    struct TRecalcParams {
        struct TParams {
            Threshold : double;
        };
        Params : TParams;
        ProdBundleParams : any;
    };
};

struct TTwoCalcersUnion {
    AdditionalBundle (required) : TBundle;
    ProdBundle (required): TBundle;
    Priority : string (allowed = ["prod", "additional"]);
};

struct TClickIntIncutMix {
    struct TParams {
        ReverseResultOrder : bool (default = false);
        UseAllDocFeatures : bool (default = false);
        RefuseThreshold : double;
        MixTopLength : ui32 (default = 5);
        MixType : string (default = "incut");
        IncutDocFeatureKeys : string[];
        OuterDocFeatureKeys : string[];
        IncutOppositePositionKey : string;
        OuterOppositePositionKey : string;
    };
    Calcer (required) : TFml;
    Params (required) : TParams;
};

struct TRandomIncutMix {
    struct TParams {
        MixTopLength : ui32 (default = 5);
        MixType : string (default = "incut");
        AdditionalSeed : string;
    };
    Params (required) : TParams;
};

struct TRandomMaskMix {
    struct TParams {
        MixTopLength : ui32 (default = 5);
        MixType : string (default = "mask");
        AdditionalSeed : string;
    };
    Params (required) : TParams;
};

struct TFeatureContext {
    AvailibleValues (required) : {string -> any};
};

struct TFeatureResult {
    Result : any;
};

struct TMultiPredict {
    Type : string;
    Data : any;
};

// multipredict for multifeature_winloss_mc
struct TScoredCategs {
    Scores : [double];
    ShowScoreThreshold : double (default = 0);
    AvailableCombinations : [bool];
    Features : [TFeature];
    AllowedCombinations : [[ui32]];
    NoPosition : [any];
};

struct TRawPredictions {
    BestCateg: i32;
    BestCategScores: {string -> double};
    AllCategScores: {string -> double[]};
    Aux: {string -> any};
};

struct TBundleInfo {
    Type : string;
    Data : any;
};

struct TMultiFeatureWinLossMcInfo {
    Features : [TFeature];
    AllowedCombinations : [[ui32]];
    NoPosition : [any];
};

struct TResult {
    IsFiltered: bool (default = false);
    IsMultiShow : bool (default = false);
    FeatureResult : {string -> TFeatureResult};
    MultiPredict : TMultiPredict;
    RawPredictions: TRawPredictions;
    Embed : [double];
    BundleInfo : TBundleInfo;
};

struct TMeta {
    RandomSeed : string;
    IsMultiPredict : bool (default = false);
    PredictedPosInfo : ui32;
    PredictedWeightInfo : double;
    PredictedFeaturesInfo : TResult;
    RawPredictions: TRawPredictions;
    FeatureContext : {string -> TFeatureContext};
    PassRawPredictions: string (allowed = ["best", "all"]);
    AddBundleInfo : bool (default = false);
    BundleInfo : TBundleInfo;

    // dictionary for pass context specific factors
    ExtraFeatures: {string -> double[]};

    // viewtype -> placement -> positions("*" or array of doubles)
    AvailableVTCombinations : {string -> {string -> any}};

    // alias -> patch
    CalcerPatch : {string -> any};

    YandexTierRelevance : double;
};

struct TCalcContext {
    Meta : TMeta;
    Result : TResult;
    Log : any;
    IsDebug : bool;
};
