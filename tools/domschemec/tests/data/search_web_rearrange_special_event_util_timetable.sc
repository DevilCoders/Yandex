namespace NSpecialEvent;

struct TTimetableSettings {
    ShowScoresInMatchName : bool;
    ShowScoresInStatus : bool;
    HighlightScores : bool;
    ShowTimeInTwoRows : bool;
    ShowLiveMatchStatusText : bool (default = true);
    ShowTodayAndTomorrowAsText : bool (default = true);
    ShowYesterdayAsText : bool;
    ShortMonth : bool;
    ShowSubscriptionIcon : bool (default = true);
    ElementsPerPage : ui64 (default = 6);
    ColumnWidth : ui64 (default = 12);
    FixedTimeWidth : ui64 (default = 0);
    Title : string (default = "Расписание");
    Stages : string[];
    ActiveStage : string;
};
