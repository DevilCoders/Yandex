namespace NBgLingBoostScheme;

struct TFacetScheme {
    Expansion : string;
    RegionId : string;
};

struct TRestrictScheme {
    Facet : TFacetScheme;
    MaxBlocks : ui32;
    MaxRequests : ui32;
    Enabled : bool;
};

struct TRemapScheme {
    Facet : TFacetScheme;
    RenormFrac : struct {
        Scale : double;
        Center : double;
        Offset : double;
    };
};

struct TLingBoostWorkerConfigScheme {
    Enabled : bool;
    BoostNoBundle : bool;
    EnableForms : bool;
    LoadFreqsInWizard : bool;
    FillLegacyFreqsInWizard : bool;
    DisableBundlesCache : bool;

    PrintRequestStr : bool;
    PrintStats : bool; //remove?
    SaasKnnEnabled : bool;
    SaasKvEnabled : bool;
    SaasKvImagesEnabled : bool;

    RestrictJson : {string -> TRestrictScheme[]};
    RemapJson : TRemapScheme[];
    SaasDispatchConfig : {string -> {string -> string[]}};

    SaasHost : string;
    SaasKvServiceId : string;
    SaasKvImagesServiceId : string;
    SaasExtraParams : string;
    SaasTimeout : string;
    SaasKnnHost : string;
    SaasKnnServiceId : string;
    SaasKnnTimeout : string;
    BundleCompression : string;
    BlocksCompression : string;

    SaasPort : ui64;
    SaasKnnPort : ui64;
    SaasKnnSearchSize : ui64;
    SaasKnnExtensions : ui64;
    SaasKnnMaxNonResponses : ui64;
    SaasKnnUseBundle: bool;
    SaasKnnEnableAlways: bool;

    SaasKnnNeighboursForQueryToText: ui64;

    Domains : string[];

    ScaleRestrict : double;

    ModifiedScheme : bool;
};
