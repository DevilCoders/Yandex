namespace NFreshManual;

struct TParams {
    SmartRelevance : bool (default = true);
    IntentThreshold1 : double (default = 0.25);
    IntentThreshold2 : double (default = 0.30);
    Degrade : double (default = 0.02);
    MaxRelev : double (default = 0.2);

    SpecialEventNews : bool;
    SpecialEventNewsSimple : bool;
    SpecialEventNewsSimpleOnTeamScreen : bool;
    SpecialEventNewsSimpleOnTimetableScreen: bool;
    SpecialEventNewsPreferSportHosts: bool;
    TimeReserve : ui64 (default = 1200);

    LimitFreshForNavQueries : bool;
    LimitFreshForNavQueriesHostThreshold : double (default = 0.3);
    LimitFreshForNavQueriesQueryThreshold : ui32 (default = 250000);

    BoostByMarkup : bool;
    FreshPolirovkaBoost : double (default = 0.4);

    HasOldYearCheck : bool (default = true);

    NewsIntoSport : bool;

    OlympiadPadFontSize : double;
    OlympiadDesktopFontSize : double;

    CleanFreshDuplicatesNews : bool (default = true);
    CleanFreshDuplicatesBiathlon : bool (default = true);

    frp : double;
    frm : double;
    frmin : double;
};
