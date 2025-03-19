namespace NWizardsClicks::NScProto;

struct TCommonCounters {
    wr (cppname = WebRequests) : i64 (default = 0);
    isr (cppname = ImagesServiceRequests) : i64 (default = 0);
    vsr (cppname = VideoServiceRequests) : i64 (default = 0);
    wcr0 (cppname = WebClickedRequests0) : i64 (default = 0);
    wcr15 (cppname = WebClickedRequests15) : i64 (default = 0);
    wcr60 (cppname = WebClickedRequests60) : i64 (default = 0);
    wcr120 (cppname = WebClickedRequests120) : i64 (default = 0);
    wwcr (cppname = WebWeightedClickedRequests) : double (default = 0.0);
};

struct TWizardCounters {
    c0 (cppname = Clicks0) : i64 (default = 0);
    c15 (cppname = Clicks15) : i64 (default = 0);
    c60 (cppname = Clicks60) : i64 (default = 0);
    c120 (cppname = Clicks120) : i64 (default = 0);
    wc (cppname = WeightedClicks) : double (default = 0.0);
    sh (cppname = Shows) : i64 (default = 0);
    csh (cppname = CorrectedShows) : i64 (default = 0);
    skp (cppname = Skips) : i64 (default = 0);
    ls (cppname = Losses) : i64 (default = 0);
    wn (cppname = Wins) : i64 (default = 0);
};

struct TNavigCounters {
    c0 (cppname = Clicks0) : i64 (default = 0);
    sh (cppname = Shows) : i64 (default = 0);
};

struct TVideoCounters {
    win (cppname = Win) : double (default = 0.0);
    loss (cppname = Loss) : double (default = 0.0);
    shows (cppname = Shows) : i64 (default = 0);
    cl (cppname = Clicks) : i64 (default = 0);
    claf (cppname = ClicksAfter) : i64 (default = 0);
    clh (cppname = ClickedHigher) : i64 (default = 0);
    views (cppname = Views) : i64 (default = 0);
    ref (cppname = Refuses) : i64 (default = 0);
    lvtwh (cppname = LVTWhenWasHigher) : double (default = 0.0);
};

struct TSurplusCounters {
    wn (cppname = Win) : double (default = 0.0);
    ck_wn (cppname = ClickWin) : double (default = 0.0);
    nck_wn (cppname = NoClickWin) : double (default = 0.0);
    ls (cppname = Loss) : double (default = 0.0);
    ck_ls (cppname = ClickLoss) : double (default = 0.0);
    rr_ls (cppname = ReReLoss) : double (default = 0.0);
    sck_pt (cppname = ShortClickPenalty) : double (default = 0.0);
    cnt (cppname = Count) : i64 (default = 0);
    r_cnt (cppname = ReqCount) : i64 (default = 0);
    sm_ps (cppname = SumOfPs) : i64 (default = 0);
};

struct TBNACounters {
    win (cppname = Win) : double (default = 0.0);
    loss (cppname = Loss) : double (default = 0.0);
    shows (cppname = Shows) : double (default = 0.0);
    surplus (cppname = Surplus) : double (default = 0.0);
};

struct TCounterContainer {
    wizard (cppname = Wizards) : {string -> TWizardCounters};
    pos_wizard (cppname = PosWizards) : { string -> {i64 -> TWizardCounters} };
    navig (cppname = Navigs) : {string -> TNavigCounters};
    common (cppname = Common) : TCommonCounters;
    video (cppname = Video) : {string -> TVideoCounters};
    intent_ver_surpus (cppname = IntentVersionedSurplus) : {string -> {string -> TSurplusCounters } }; // intent -> version -> surplus
    bna (cppname = Bna) : TBNACounters;
};

