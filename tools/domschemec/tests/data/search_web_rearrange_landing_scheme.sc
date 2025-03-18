namespace NLandingRule;

struct TBlendPolicy {
    BanOrigWizard : bool (default = false);
    BlendOnlyIfNoOrigWizard : bool (default = false);
};

struct TIntentConfig {
    Enabled : bool (default = false);
    UseUserGrouping : bool(default = false);
    CreateBlenderFactors : bool(default = true);
    SrcWizardGrouping (required) : string;
    EasyBlendConfigPath (required) : string;
    Policy : TBlendPolicy;
    Top : ui32 (default = 20);
    ConstrainPrsPos: bool (default = false);
    MaxPrsPos : ui32 (default = 500);
    MaxDocs : ui32 (default = 1);
    DisabledReasons : {string -> bool};
    ConfigPatch : any;
};

struct TLanding {
    Enabled : bool(default = false);
    TargetGrouping (required) : string;
    LandingConfig : {string -> TIntentConfig}; // intent -> config
};
