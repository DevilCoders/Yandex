namespace NNewsFromQuickMiddle;

struct TMultipleStoryModeOptions {
    Enabled : bool;

    ShowGreenUrl : bool (default = true);
    ShowPictures : bool (default = true);

    ViewType : string;

    MinimalDocumentCount : ui32 (default = 0);
    MaximumDocumentCount : ui32 (default = 3);
};

struct TOptions {
    Enabled : bool;

    PictureInOrganicFreshness : bool;

    FirstSnippetMinimalLength : ui32;
    AllowNoImage : bool;
    ForcePictureDocument : bool;
    MinimalDocumentsCount : ui32;
    PreferPictureDocuments : bool;
    FilterNewsWithoutImage : bool;
    UseLogoForAllDocs : bool;
    UseLogoAlways : bool;
    TurboSportNewsOnly : bool;
    TurboNewsOnly : bool;
    SportWizardNewsCount : ui32 (default = 3);
    WizardDocumentsCount : ui32 (default = 3);

    MinimalPicturesCount : ui32;
    UseWizardPictureForDocuments : bool;

    MultipleStoryModeOptions : TMultipleStoryModeOptions;

    MinTopNewsPositionForSingleStory : ui32 (default = 2);
    MinNewsForSingleStory : ui32 (default = 2);
    MaxNewsForSingleStory : ui32 (default = 3);
    ShowSingleStory : bool;
    SingleStoryViewType : string (default = "story");
    ForcedBlenderViewType : string;
    ViewType : string;

    UseHeadlineSnippets : bool;
    UseNewsSerp : bool;

    TitleType : string;
    HighlightText : bool;

    AgeToDeleteDocIfNoQueryData : ui64;

    NewsStoryUrl : string;
    NewsStoryPdaUrl : string;
    NewsStoryGreenUrl : string;

    UseRawNewsPictures : bool;
    UseClusterPictures : bool;
    UseTurboPictures : bool;
    UseVideoSnapshots : bool;
    UseVideoSnapshotsFromVideohosting : bool;
    PreferVideoFromVideohosting : bool (default = true);

    NewsHostsList : string (default = "news_hosts");

    MinTitleHighlightedWords : ui64;

    RemoveNewsWizardIfCannotClusterize : bool;
    AskLongTitlesFromQuick : bool;
    QuickMaxTitleLen : ui64 (default = 300);
    DumpRandomLogData : bool;

    PictureHeight : ui64 (default = 100);
    PictureWidth : ui64 (default = 100);

    DesktopPictureHeight : ui64 (default = 200);
    DesktopPictureWidth : ui64 (default = 270);
    DesktopLogoHeight : ui64 (default = 72);
    DesktopLogoWidth : ui64 (default = 72);

    PadPictureHeight : ui64 (default = 200);
    PadPictureWidth : ui64 (default = 270);
    PadLogoHeight : ui64 (default = 144);
    PadLogoWidth : ui64 (default = 144);

    TouchPictureHeight : ui64 (default = 240);
    TouchPictureWidth : ui64 (default = 480);
    TouchListPictureHeight : ui64 (default = 72);
    TouchListPictureWidth : ui64 (default = 72);
    TouchShowcasePictureHeight : ui64 (default = 149);
    TouchShowcasePictureWidth : ui64 (default = 198);

    TouchRetinaPictureHeight : ui64 (default = 450);
    TouchRetinaPictureWidth : ui64 (default = 600);
    TouchRetinaListPictureHeight : ui64 (default = 144);
    TouchRetinaListPictureWidth : ui64 (default = 144);
    TouchRetinaShowcasePictureHeight : ui64 (default = 298);
    TouchRetinaShowcasePictureWidth : ui64 (default = 396);
};
