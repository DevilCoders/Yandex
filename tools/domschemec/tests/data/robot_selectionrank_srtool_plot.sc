namespace NPlot;

struct TPlot {
    name (required) : string;
    filter (required) : string;
};

struct TPlots {
    plots : [TPlot];
};
/*
struct TCounter {
    name (required) : string;
    count (required) : int;
};

struct TCounters {
    counters : [TCounter];
};
*/