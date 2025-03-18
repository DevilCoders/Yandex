namespace NBigNavAnswerUpperSc;

struct TSnippetCutSettings {
    Rows : double (default = 2);
    Factor : double (default = 1);
    Padding : double (default = 50);
};

struct TRuleConfig {
    // content properties
    MaxUrlsNm (required) : ui32;
    SnippetKey (required) : string;
    UseBlackList : bool (default = true);
    UseWhiteList : bool (default = true);
    InBlackList : bool (default = false);
    InWhiteList : bool (default = false);
    TurnOff : bool (default = false);
    EnableOrgWizTweak : bool (default = true);
    CutSnippet : bool (default = false);
    SnippetCutSettings : TSnippetCutSettings;

    // blender properties
    DisableBlender : bool (default = false);
    IntentName (required) : string;
    DocRelevance : double (default = 0.2);
    ViewTypeFeature (required): string;
    ForbiddenViewTypes : {string -> bool};
    EasyBlendCfg : any;

    // common
    Verbose : bool (default = false);
};
