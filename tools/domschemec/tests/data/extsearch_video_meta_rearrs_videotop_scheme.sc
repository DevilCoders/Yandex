namespace NVideo;

struct TVideotopMiddleScheme {
    Enabled: bool (default = true);

    FilterVtop: bool (default = false);
    FilterTopThumbs: bool (default = true);
    FilterThumbs: bool (default = false);
    PreserveOrder: bool (default = false);

    RandomSerp: bool (default = false);
    CandTopN: ui32 (default = 60);
    RearrTopN: ui32 (default = 6);
    ModTimeBorder: double (default = 0.99);
    MaxAge: double (default = 0.74);

    RandomRelated: bool (default = false);
    RelatedCandTopN: ui32 (default = 60);
    RelatedRearrTopN: ui32 (default = 10);
    RelatedModTimeBorder: double (default = 1.);
    RelatedMaxAge: double (default = 0.);

    Salt: ui32 (default = 1);

    FastDelivery: bool (default = true);
    TrieMaxUrls: ui32 (default = 20);
    QueryKvSaas: bool (default = true);
    KvSaasService: string (default = "video_top");
    KvSaasPrefix: string (default = "");
    TrieQuery: string;
    DeliveryMethod: string;
    SoftDeliveryMethodForPersTop: bool (default = true);

    Titles: bool (default = true);
    AllLangTitles: bool (default = false);
};

struct TFixedUrls {
    urls (cppname = Urls): string[];
    mbu_urls (cppname = MbuUrls): string[];
};
