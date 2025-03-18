namespace NTvBroadcasting;

struct TNotLiveVideoParams {
    Enabled : bool (default = false);
    VideoCaption : string (default = "Яндекс.Видео");
    MaxVideosToCheck : ui64 (default = 5);
    VideoGreenUrlTitle : string (default = "Яндекс");
    VideoGreenUrlSubTitle : string (default = "Эфир");
};

struct TRecommendationsParams {
    Enabled : bool (default = false);
    MoreDataUrl : string (default = "//yandex.ru/?stream_channel=100000&stream_active=storefront");
    MinClipCount : ui64 (default = 3);
    MaxClipCount : ui64 (default = 6);
    CounterPath : string (default = "//tv-online/recommendations");
};

struct TSportVideoParams {
    Enabled : bool (default = false);
    VideoGreenUrlTitle : string (default = "Яндекс");
    VideoGreenUrlSubTitle : string (default = "Эфир");
    CarouselIdPrefix : string (default = "tv_sport_");
    MaxTeamIdsPerLemma : ui64 (default = 5);
    MaxTeamIdsTotal : ui64 (default = 8);
    AllowedSports : any;
};

struct TParams {
    Enable (cppname = Enabled) : bool;

    SaasNamespace : string (default = "tv_shows");  // for testing use "test_tv_shows"

    UseVhData : bool (default = false);
    ForceUsingVhData : bool (default = false);
    SkipNonOnlineVhVideo : bool (default = false);

    ShowSpecialEvents : bool (default = false);
    SpecialEventsUseClassifier : bool (default = true);
    SpecialEventMaxPosition : ui64 (default = 100);

    SkipTvExpChannelRule : bool (default = true);
    BannedChannels : string[];

    SkipTvShows : bool;
    SkipTvProgram : bool (default = true);
    SingleChannelSnippetPosition : ui64 (default = 0);
    MaxProgramStartDelay : ui64 (default = 300);

    AllowedBrowsers : any;

    EnableForAll (cppname = CommonSnippetEnabled) : bool;
    SkipTvShowsAll : bool;
    CommonSnippetPosition : ui64 (default = 0);

    CheckTimeZone (cppname = CheckTimezone) : bool;
    // We should get Moscow region id from geobase
    MoscowRegionId : ui64 (default = 213);

    MinPercentMatch (cppname = MinProgramPercentMatch) : double (default = 0.5);

    DeriveChannelFromProgram : bool;
    ProgramQueryBlacklistedWords : any;
    ProgramSimilarityThreshold : double (default = 0.8);
    UseSingleWordQueries : bool;
    PrecalculateQueryEmbedding : bool;

    CheckCountry : bool;
    CheckCountryWithChannel : bool;
    AvailableCountries : i64[];

    EntityScheduleTitle : string (default = "Телепрограмма");

    SwapTitleAndSubtitleForFutureShows : bool;
    ShowTitleMaxCharacters : ui64 (default = 56);

    ShowCommonSnippetAsWizardCarousel : bool;
    CommonSnippetWizardTitle : string (default = "Эфир");
    UseChannelLinkInCommonSnippetTitle : bool;
    CommonSnippetBlockData : any;
    CommonSnippetColorScheme : any;
    CommonWizardCarouselId : string (default = "tv_online");
    CommowWizardOpenNewWindow : bool;
    UseNewCarouselDataFormat : bool;
    NewCarouselDataFormatMaxPrograms : ui64 (default = 4);
    GreenUrlTitle : string (default = "Яндекс");
    GreenUrlSubTitle : string (default = "Эфир");

    BlenderConfig : any;
    UseEasyBlender : bool; // temp flag for transition period
    // Used in continuous experiment for blender formula
    // show tv on a very wide range of requests in order to collect representative pool
    ShowTvVeryOften : bool;
    CalculateBlenderDynamicFactors : bool;

    ExtractShowsFromSnippets : bool (default = false);
    MaxSnippetLookup : ui64 (default = 10);
    UseWizDetectionRule : bool (default = false);

    AddProgramScheduleToChannel : bool (default = true);

    AssistantStartingProgramDelay : ui64 (default = 60);
    AssistantNextProgramDelay : ui64 (default = 600);
    AssistantTvProgramsAllowed : bool (default = false);

    NotLiveParams : TNotLiveVideoParams;
    RecommendationsParams : TRecommendationsParams;
    SportVideoParams : TSportVideoParams;

    ShowAutoChannels : bool;

    DisableTvForFilmsInES : bool (default = false);

    ShowChannelsWithTimezones : bool (default = false);
    MaxTimezoneDifference : i32 (default = 0);
};
