namespace NNewsFromQuickUpper;

struct TViewType {
    Property : string;
    Value : string;
    ViewType : string;
};

struct TViewTypeLimits {
    MinDocCount : ui32;
    MaxDocCount : ui32;
};

struct TParams {
    MinimalDocumentsCount : ui64 (default = 0);

    WizardDocumentsCount : ui64 (default = 3);

    NoImages (cppname = NoImagesWizard) : bool (default = false);

    ForcedBlenderViewType : string;

    FilterSportNewsInNewsWizard : bool (default = false);

    ViewTypeAutoSelect : bool (default = false);

    LogWizardDocumentsUrls : bool (default = true);
    DocsPropertiesToLog : string[];

    CacheKeyDocCount : ui64 (default = 3);

    PrepareCacheKey : bool (default = false);

    DefaultViewType : string (default = "");
    NoImagesViewType : string (default = "");
    UseCorrectNewsUrl : bool (default = false);
    Enabled : bool (default = false);
    QuickNewsExpSource : bool (default = false);

    newswizard (cppname = BlenderConfig) : any;

    EnableSmartListLeft : bool (default = false);
    ListLeftDocumentsCount : ui64 (default = 3);
    ListLeftPicturesCount : ui64 (default = 2);

    ViewTypes : TViewType[];
    ViewTypeLimits : { string -> TViewTypeLimits };

    ReHighlightDocuments : bool (default = false);
    ReHighlightTitles : bool (default = false);
};
