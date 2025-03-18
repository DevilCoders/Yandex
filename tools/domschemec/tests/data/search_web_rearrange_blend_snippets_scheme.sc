namespace NBlendSnippetsSc;

struct TSnippetParams {
    // pretend EasyBlend vertical
    IntentWeightFml : string;
    IntentName (required) : string;

    // gta Snippet key
    ContentKey (required) : string;

    // feature result params
    ShowFeature : string (default = "Show");
    ShowByDefault : bool (default = true);

    // for fix list through fconsole
    InWhiteList : bool (default = false);
    InBlackList : bool (default = false);
};

struct TRuleConfig {
    Snippets : {string -> TSnippetParams};
    UseFixLists : bool (default = true);
    Verbose : bool (default = false);
};
