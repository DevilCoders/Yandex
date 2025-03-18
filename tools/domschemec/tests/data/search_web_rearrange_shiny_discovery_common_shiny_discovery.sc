namespace NShinyDiscovery;

struct TParams {
    Enabled : bool;
    SaasNamespace : string (default = "shiny_discovery");
    DocCateg : string (default = "ShinyDiscoveryCateg");
    InsertPos : ui32;

    UseEasyBlend : bool (default = true);
    Grouping : string (default = "shiny_discovery");
    EasyBlendCfg : any;

    DoHighlight : bool (default = false);
    HighlightByRecommendedQuery : bool (default = false);
    LogFields : {string -> bool};
};
