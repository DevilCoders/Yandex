namespace NFresh;

struct TParams {
    //------------------------------------------
    // Samohod fresh params

    // Позволяет (для экспериментов отключить ранжирование на среднем)
    // то есть не посылать дополнительные параметры в источник WEB
    // Для отключения надо явно выставить в false.
    SamohodOnMiddle : bool (default = true);

    // Содержит список cgi-параметров, которые надо дописать к источнику WEB,
    // чтобы включить ранжирование на среднем.
    SamohodOnMiddleParams : struct[] (array) {
        Key : string (required, != "");
        Value : string (required);
    };

    //------------------------------------------
    // Callisto fresh params

    CallistoOnMiddle : bool (default = true);

    // Содержит список cgi-параметров, которые надо дописать к источнику WEB,
    // чтобы включить ранжирование на среднем.
    CallistoOnMiddleParams : struct[] (array) {
        Key : string (required, != "");
        Value : string (required);
    };

    DisableAllImprovementsSixMonth : bool;

    ChangeQtreeForNews : bool;
    ChangeQtreeForQuick : bool;
    WaresFilmThreshold : double;
    CheckQtreeParamsForNews : bool;
    CheckQtreeParamsForQuick : bool;

    window : double;

    QuickRelevFml : string;

    SeparateSamohod : bool;

    GroupAll : bool;

    TimeMode : ui64;
    EventStart : ui64;

    RelevExperiment : bool;
    RelevControl : bool;
    RelevExpFormula : string;

    FilterFreshHosts : bool;
    FreshGroupingSize : ui64 (default = 15);

    NewsAgenQualityPPThreshold : double (required);

    DirectOff : bool;

    IsBlenderStage : bool;

    DropFreshDates : bool;

    TimeWindowThreshold : ui64;

    PrepareCacheKey : bool;
    CacheKeyDocCount : ui64 (default = 3);

    DumpWebUrls : bool;
    DumpFreshUrls : bool;

    NewsInsteadOfQuick : {string -> bool};

    BoostAfterEventDisallow : {string -> bool};

    EraseSamohod : struct {
        Enabled : bool;
        JustForCommercial : bool;
        JustForGeoLocal : bool;
        JustForNotNav : bool;
    };

    LuaBoosts : struct[] {
        "Report.AcceptedRules" (cppname = AcceptedRules) : string;
        Sources : {string -> bool};
        LuaBoost : string;
    };

    // Временные параметры для эксперимента в рамках SEARCH-7215
    // Позволяют запросить вместо стандартной группировки кастомную
    // Предназначено для работы в паре с подменой источника
    GroupingDFresh : string;
    GroupingDFreshExp : string;
};
