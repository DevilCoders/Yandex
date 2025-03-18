namespace NImgCommercial;

struct TImgCommercialScheme {
    Enabled : bool (default = true);
    EnabledTLDs : string (default = "ru"); // ','-separated
    DisableRegionValidation : bool (default = false);
    MaxOffersCountForOneDocument : ui32 (default = 2);
    MaxOffersCountForOneDocumentOverride : any; // int number or dict {"<source>": <number>, "default": <number>}
    ParallelValidationRequestsMaxCount : ui32 (default = 5);
    ValidationOffersMaxCount : ui32 (default = 30);

    EnableCommercialFilter : bool (default = false);
    AutoEnabled : bool (default = false);
    UseRegionRestriction : bool (default = true);
    UnanswerThreshold : ui32 (default = 5);

    MaxDocsInDaemonRequest : ui32 (default = 7);

    MoneySource : string (default = "2");
    Regions : ui32[];

    FirstRequiredDocPos : ui32;
    LastRequiredDocPos : ui32;
    GroupCountDeltaRatioToValidate : double (default = 0.0);

    FixCountMinSize : ui32 (default = 500);
    FixCountMinMult : double (default = 0.8);

    UserIP : string;
    UserAgent : string;
    Cookie : string;

    DirectTitlesRequestPageId : string;

    // https://st.yandex-team.ru/IMAGES-13325
    EnableSimilarOffersInfestation : bool (default = false);

    // use only market offers (pictures with market tray markers on serp) to check for infestation possibility
    CountMarketOffersOnly : ui32 (default = false);
    MinSimilarOffersToInfest : ui32 (default = 5);
    MaxSimilarOffers: ui64 (default = 4);

    // smart banners https://st.yandex-team.ru/IMAGES-13619
    DirectSmartPage : string (default = "248375");
    DirectSmartImpId : ui32 (default = 2);
    ValidateDirectSmart : bool (default = true);

    // https://st.yandex-team.ru/IMAGES-14258
    EnableClothesCategoriesFilter : bool (default = false);

    // https://st.yandex-team.ru/IMAGES-14359
    AllExceptMarketIsSimilar : bool (default = false);
    ShowSingleSourcePerDocument : bool (default = false);
    SchemaOrgBeforeMarket : bool (default = false);

    // https://st.yandex-team.ru/IMAGES-14796
    RemoveDuplicatedUrls : bool (default = false);
};
