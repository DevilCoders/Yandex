namespace NBASSConfig;

struct TYdbConfig {
    DataBase : string (required);
    Endpoint : string (required);
    Token : string;
};

struct TConfig {
    struct TSource {
        EnableFastReconnect : bool (default = false);
        External            : bool (default = false);
        Host (required)     : string;
        MaxAttempts         : ui16 (default = 1);
        RetryPeriod         : duration (default = "50ms");
        Timeout             : duration (default = "150ms");
        Tvm2ClientId        : string;
    };

    struct THeader {
        Name (required) : string;
        Value (required) : string;
    };

    struct TVins {
        struct TWeatherNowcastSource : TSource {
             StaticApiKey : string (required);
        };
        struct TTvm2Source : TSource {
            BassTvm2ClientId : string (required);
            UpdatePeriod : duration (default = "1h");
        };
        struct TNewsSource : TSource {
            RefreshTime : duration (default = "10m");
        };
        struct TExternalSkill : TSource {
            CacheTimeout : duration (default = "1h");
            SkillTimeout : duration (default = "500ms");
            AvatarsHost  : string (default = "avatars.mds.yandex.net");
        };
        struct TPassportSource : TSource {
            Consumer : string;
        };
        struct TKinopoiskSource : TSource {
            ClientId : string (required);
        };

        AbuseApi : TSource;
        Afisha (required) : TSource;
        BlackBox (required) : TSource;
        Weather (required) : TSource;
        WeatherNowcast (required) : TWeatherNowcastSource;
        TVGeo (required) : TSource;
        TVSearch (required) : TSource;
        TVSchedule (required) : TSource;
        GeoMetaSearch (required) : TSource;
        MapsApi (required) : TSource;
        StaticMapRouter (required) : TSource;
        GeoCoder (required) : TSource;
        Search (required) : TSource;
        ReqWizard (required) : TSource;
        UnitsConverter (required) : TSource;
        CarRoutes (required) : TSource;
        RouterVia (required) : TSource;
        MassTransitRoutes (required) : TSource;
        Passport (required) : TPassportSource;
        PedestrianRoutes (required) : TSource;
        PersonalData (required) : TSource;
        MapsInfoExport (required) : TSource;
        NerApi (required) : TSource;
        News (required) : TSource;
        NewsApi (required) : TNewsSource;
        Music (required) : TSource;
        MusicQuasar (required) : TSource;
        MusicSuggests (required) : TSource;
        MusicCatalog (required) : TSource;
        RadioAccount (required) : TSource;
        RadioDashboard (required) : TSource;
        RadioStream (required) : TSource;
        TrafficForecast (required) : TSource;
        Tvm2 (required) : TTvm2Source;
        VideoAmediateka (required): TSource;
        VideoHostingTvChannels (required): TSource;
        VideoHostingTvEpisodes (required): TSource;
        VideoIvi (required): TSource;
        VideoKinopoisk (required): TKinopoiskSource;
        VideoKinopoiskJson (required) : TSource;
        VideoTop (required): TSource;
        VideoYandexXmlSearch (required): TSource;
        VideoYouTube (required): TSource;
        QuasarBillingPromoAvailability (required): TSource;
        QuasarBillingAvailability (required): TSource;
        QuasarBillingContentBuy (required): TSource;
        QuasarBillingContentPlay (required): TSource;
        QuasarBillingSkills (required): TSource;
        ComputerVision (required): TSource;
        ComputerVisionClothes (required): TSource;
        CalendarHolidays: TSource;
        Market (required) : TSource;
        MarketHeavy (required) : TSource;
        MarketBlue (required) : TSource;
        MarketBlueHeavy (required) : TSource;
        MarketCheckouter (required) : TSource;
        MarketCheckouterIntervals (required) : TSource;
        MarketCheckouterOrders (required) : TSource;
        MarketFormalizer (required) : TSource;
        MarketMds (required) : TSource;
        CalendarApi (required): TSource;
        RemindersApi (required): TSource;
        SocialApi : TSource;
        AviaBackend : TSource;
        AviaSuggests : TSource;
        AviaPriceIndex : TSource;
        AviaPriceIndexMinPrice : TSource;
        ExternalSkillsApi : TExternalSkill;
        ExternalSkillsKvSaaS : TSource;
        ExternalSkillsSaaS : TSource;
        ExternalSkillsRecommender : TSource;
        WebDiscovery (required) : TSource;
        WebDiscoveryBeta (required) : TSource;
        ZoraProxy : struct {
            Host : string (default = "zora-online.yandex.net:8166");
            SourceName : string (default = "bass");

            (validate Host) {
                if (Host()->Contains("://")) {
                    AddError("must be 'host[:port]', without scheme");
                }
            };
        };
        Translate : TSource;
        TranslateDict : TSource;
        TranslateMtAlice : TSource;
        TranslateTranslit : TSource;
        NormalizedQuery : TSource;
        TankerApi : TSource;
        TaxiApi : TSource;
    };

    struct TRedirectApi {
        ClientId : string (required);
        Key      : string (required);
        Url      : string (required);
    };

    struct TMarketSettings {
        UseBeruTestingUrls : bool (default = false);
        MaxCheckoutWaitDuration: duration (default = "15s");
    };

    HttpPort (required) : ui16;
    HttpThreads : ui16 (default = 10);
    SetupThreads : ui16 (required);
    CacheUpdatePeriod : duration (default = "5m");

    EventLog : string;

    UpdateVideoContentDb : bool (default = false);
    VideoContentDbUpdatePeriod : duration (default = "1h");

    AvatarsMapFile : string;
    EnableBlobCards : bool (default = false);
    RedirectApi (required) : TRedirectApi;
    DialogsStoreUrl (required) : string;
    OnboardingUrl (required) : string;
    OnboardingGamesUrl (required) : string;

    Vins (required) : TVins;

    FetcherProxy : struct {
        HostPort (required) : string;
        Headers : [ THeader ];
    };

    SkillStyles : { string -> any };

    PersonalizationNameDelay : duration (default = "3h");
    PersonalizationBiometryScoreThreshold : double (default = 0.9);
    PersonalizationAdditionalDataSyncTimeout : duration (default = "50ms");
    PersonalizationDropProbabilty : double (default = 0.66);

    Market : TMarketSettings;

    YDb : TYdbConfig;
    TestUsersYDb : TYdbConfig;

    YdbConfigUpdatePeriod: duration (default = "5m");
};
