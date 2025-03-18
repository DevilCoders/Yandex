namespace NContentGen;


struct TContentGen {
    Enabled : bool(default = false);
    TargetGrouping : string(default = "d");
    WebTop : ui32(default = 15);
    HqcgTop : ui32(default = 5);
    PersonalThreshold : double(default = 0.1);
    PersonalRankingFeature : string(default = "TextBM25");
    ForcePersonalRanking : bool (default = false);
    ModelName : string;
    ModelThreshold : double (default = 0.5);
    Intent : string (default = "CONTENTGEN");
    MaxPosToInsert : ui32 (default = 3);
    CommercialPos : ui32 (default = 5);
    CommericalThreshold : double (default = 0.2);
    MaxDocsToInsert : ui32 (default = 1);
    Simulate : bool (default = false);
    CheckUATraits : bool (default = true);
    UseDefaultTitle : bool (default = true);
    TalkProfileUrl : string (default = "https://yandex.ru/talk/user/");
    OnlyPrecomputedQueries : bool (default = false);
    MinDocCharSize : i32 (default = 1000);
    DssmThreshold : double (default = 0.8);
};
