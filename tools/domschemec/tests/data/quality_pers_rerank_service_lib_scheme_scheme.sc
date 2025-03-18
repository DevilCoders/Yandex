namespace NPers::NAtomServiceScheme;

///////////////////////// RERANK /////////////////////////

struct TBoostData {
    prior-power : ui64;
    default-scores : [string];
};

struct TFormula {
    name : string (default = "");
    single : bool (default = true);
    cpf : bool (default = false);
    mult : double (default = 1.);
    id : ui64 (default = 0);
};

struct TFormulaWithCoef {
    formula : TFormula;
    coef : double;
};

struct TSubst {
    filter : string;
    data : string;
};

struct TFeatureFilter {
    fidx : ui32;
    lb : double (default = 0);
    ub : double (default = 1);
    negate : bool;
};

struct TRtmrboostDistrData {
    host-shows : {string -> ui32};
};

struct TPersCandidateScores {
    host-scores : {string -> string};
    url-scores : {string -> string};
};

struct TSourceConfig {
    should-fetch : bool (default = true);
    force-fetch-userdata : bool (default = false);
    path : string;
    url : string;
};

struct TEntityExtraFeature {
    url : string;
    freq : ui32;
    wtype : string;
};

struct TCandidate {
    filter : string;
    url : string;
    internal-url : string;
    product : string;
    grouping-key : string;
    title : string;
    snippet : string;
    lang : ui32;
    langname : string;
    aux-data : {string -> string};
    weight : double (default = 1);
    bandits-weight : double;
    bandits-weight-expression : string;
    fml-weight-expression : string;
    fml-weight : double;
    mandatory : bool (default = false);
    max-shows: ui64;
    scores : [string];
    text-subst : {string -> [TSubst]};
    subst-fields : [string];
    ignore-default-filters : bool (default = false);
    ignore-hide : bool (default = false);
    obj-id : string;
    extra : [TEntityExtraFeature];
    background : string;
    veil : string;
    type (cppname = InstallerType) : string;
};

struct TDegradationConfig {
    never-degrade : bool (default = true);
    always-degrade : bool (default = false);

    time-before-rescale-ms : ui32 (default = 100);
    alpha : double (default = 0.05);

    top-size : ui32 (default = 0);
    threshold : double (default = 0.9);
    factor : double (default = 1.);
};

struct TFilterParams {
    client-id : string;
    subclient-id : string;
};

struct TThresholdRequest {
    formula-names : [string];
    adjust-percentage : bool (default = true);
    adjustment-power : double (default = 0.);
    show-percentage : double (default = 1.);
    ignore-pass-stages-probabilities : bool (default = false);
};

struct TExperimentRequest {
    threshold-request : TThresholdRequest;
    drop-formulas : [string];
    dump-request : bool (default = false);
};

struct TExperimentParams {
    collect-formula-names : [string];
};

struct TFmlBanditParams {
    fml : TFormula;
    bandits-weight : double;
    bandits-weight-expression : string;
};

struct TFmlBanditContext {
    fml-bandit-params : [TFmlBanditParams];
    bandits-counter : string;
    bandits-expression : string;
};

struct TSmartRequest {
    type : string;
    name : string;
    uid : string;
    uuid : string;
    gid : ui64;
    real-ip : string;
    wifi : string;
    cellid : string;
    filtered : bool (default = false);
    reqid (cppname = ReqID) : string;
    showid (cppname = ShowId) : string;
    optimize : bool (default = true);
    describe-features : bool (default = true);
    precalced-features : [[double]];
    fastfdump : bool (default = false);
    dbg : bool (default = false);
    group : ui32 (default = 0);
    show-features : bool (default = false);
    show-features-by-id : [ui16];
    show-features-by-name : [string];
    show-boost-features : bool (default = false);
    show-boost-features-by-id : [ui16];
    show-boost-features-by-name : [string];
    training-sample-portion : double (default = 0.);
    boost : [double];
    user-agent : string (default = "");
    ua-traits : {string -> string};
    req-params : {string -> string};
    headers : {string -> string};
    yp-cookie : string;
    yp : string (default = "");
    yc : string (default = "");
    ys : string (default = "");
    ycookie : string;
    referer : string (default = "");
    referer-origin: string (default = "");
    tld : string (default = "");
    lr : ui32;
    return-metadata : bool (default = false);
    require-userdata : bool (default = true);
    prefer-boost-fml : bool (default = true);
    boost-fml : [double];
    boost-data : TBoostData;
    urls : [TCandidate];
    sdr : string;
    relev : string;
    query : string;
    service-host : string;
    ts : ui64;
    test-ids : [ui64];
    use-local-ws-calc : bool (default = false);
    sources : {string -> TSourceConfig};
    ignore-rtmr-tables : [string];
    aux-counters : [string];
    test-mode : bool (default = false);
    test-candidates-mode : bool (default = false);
    smart-conflict-resolution : bool (default = false);
    bandits : bool (default = false);
    always-fill-bandit-scores : bool (default = false);
    bandits-debug : bool (default = false);
    client-settings : [TSmartRequest];
    stored-candidate-list : string;
    bandits-counter : string;
    bandits-expression : string;
    default-filters : string (default = "default");
    fetch-boost-scores-in-batch-mode : bool (default = false);
    loginhash : string;
    threshold : double;
    ignore-last-action (cppname = ShowHidden) : bool (default = false);
    fetch-userdata : bool (default = false);
    max-hf : ui64 (default = 0);
    analyze-data-impact : bool (default = false);
    analyze-feature-impact : bool (default = false);
    show-candidates : bool (default = false);
    show-filtered-candidates : bool (default = false);
    no-error-on-empty-userdata : bool (default = false);
    lang : ui64;
    clid : string (default = "");
    clid-type : string (default = "");
    langname : string;
    slow-connection : bool;

    custom-fml : TFormula;
    custom-fml-ensemble : [TFormulaWithCoef];

    feature-filters : [TFeatureFilter];
    rtmrboost-distr-data : TRtmrboostDistrData;
    pers-candidate-scores : TPersCandidateScores;
    force-candidates : [string];
    ban-candidates : [string];
    timeout : ui32 (default = 100);
    aux-info-max-feature-vecs : ui32 (default = 20);
    aux-info-dump-features : bool (default = false);
    extra-candidate-count : ui16 (default = 0);
    complex-conflict-resolution : bool (default = false);
    subrequest-dcg-discounts : [double];
    top-size : ui32 (default = 30);
    max-results : ui32;
    preselection-list-size : ui32;
    truncate-non-mandatory : bool (default = false);

    prefiltration-fml : string;
    use-prefiltration-show-percentage : bool (default = true);
    prefiltration-threshold : double (default = 0);
    prefiltration-show-percentage : double;
    prefiltration-adjust-percentage : bool (default = true);
    prefiltration-adjustment-power : double (default = 0.);

    do-not-degrade : bool (default = false);
    degradation-config : TDegradationConfig;

    show-local-userdata : bool (default = false);
    max-local-and-remote-ts-diff : ui32 (default = 10);
    track-in-local-storage : bool (default = false);
    filter-params : TFilterParams;

    experiment-params : TExperimentParams;

    use-show-percentage : bool (default = true);
    show-percentage : double;
    adjust-percentage : bool (default = true);
    adjustment-power : double (default = 3.);
    ignore-pass-stages-probabilities : bool (default = false);

    ranked-quota : string (default = "");
    use-ranked-quota : bool (default = true);

    entity-search-request : bool (default = false);

    use-subst-bandit-stats : bool (default = false);

    write-substs-in-vars : bool (default = true);
    write-adata-in-vars : bool (default = true);

    min-user-id-age : ui32 (default = 0);
    use-aggregated-stats : bool (default = 0);

    fml-bandit-context : TFmlBanditContext;
};

struct TAppHostSmartRequestCtx {
    name : string;
    meta : {string -> bool};
    results : [any];
};

struct TRegionResolverConfig {
    region2genitive : string;
    default-region-genitive : string;
};

struct TCountersConfig {
    pid : string;
    cid : string;
    path-prefix : string;
};

struct TFeatCalcConfig {
    task-count : ui32 (default = 1);
    mult : ui32 (default = 2);
    ec-mult : ui32 (default = 5);
    parallel-feat-calc-threshold : ui32 (default = 8);
};

struct TListPreselectionConfig {
    max-list-size-on-load : ui32 (default = 300);
    default-counter : string;
    default-scores : [string];
    buckets : struct {
        counter : string;
        prior : [ui64];
    };
};

struct TPreselectionConfig {
    update-interval : ui32 (default = 300);
    default : TListPreselectionConfig;
    custom : [
        struct {
            lists : [string];
            config : TListPreselectionConfig;
        }
    ];
};

struct TPreselectionBucketInfo {
    top : ui32 (default = 0);
    s : double;
    beta : [ui64];
    urls : [any];
};

struct TPreselectionInfo {
    buckets : [TPreselectionBucketInfo];
};

struct TPreselectionLogEntry {
    version : ui64;
    gen : ui64;
    host : string;
    list : string;
    size : ui32;
    limit : ui32;
    truncated : ui32;
    timestamp : ui64;
    update-time : ui64;
    result : [TPreselectionBucketInfo];
};

struct TCandidateStorageConfig {
    stored-candidates-exp-time : ui32 (default = 10);
    updater-wait-on-error-time : ui32 (default = 60);
    stored-candidate-dir : string;
    word-stats-cache-size : ui32 (default = 20000);
};

struct TAppHostConfig {
    port : ui16;
    grpc-port : ui16;
    thread-count : ui16 (default = 1);
    grpc-thread-count : ui16 (default = 4);
};

struct TStatsConfig {
    interval-count : ui32 (default = 50);
    leftmost-interval : double (default = 0);
    step : double (default = 5);
};

struct TUserDataStorageConfig {
    entry-lifetime : ui32 (default = 0);
    shard-count : ui32 (default = 0);
};

struct TExperimentConfig {
    experiment-storage-update-interval_ms : ui32 (default = 2000);
    experiment-storage-wait-on-error-interval_ms : ui32 (default = 5000);
    measurement-discount-interval_ms : ui32 (default = 600000);
    measurement-discount-coefficient : double (default = 0.75);
    median-recompute-interval_ms : ui32 (default = 5000);
    max-bucket-count : ui32 (default = 100);
    seed-count : ui64 (default = 250);
    start-bucket-count : ui32 (default = 20);
    minimum-splittable-size : ui64 (default = 20);

    interval-duration_ms : ui32 (default = 500);
    interval-count : ui32 (default = 120);
};

struct TUserIDResolutionCacheConfig {
    size : ui32 (default = 1000000);
    entry-lifetime : ui32 (default = 36000);
};

struct TRtmrSetupConfig {
    user-id-resolution-cache : TUserIDResolutionCacheConfig;
};

struct TPqClientConfig {
    topic : string;
    log-type : string;
    pq-balancer : string;
};

struct TPqConfig {
    thread-count : ui32 (default = 1);
    clients : {string -> TPqClientConfig};

    event-interval_ms : ui32 (default = 1000);
    wait-on-error-interval_ms : ui32 (default = 2000);

    max-producer-creation-time_ms : ui32 (default = 10000);
    max-message-sending-time_ms : ui32 (default = 1000);
    max-messages-dequeued : ui32 (default = 2000);
};

struct TConfig {
    word-stats-cache-size : ui32;
    worker-count : ui16 (default = 1);
    max-queue-size : ui32;
    featcalc (cppname = FeatCalcConfig) : TFeatCalcConfig;
    par-src-timeout : ui32 (default = 50);
    fml-dir : string;
    fml-dirs : [string];
    fml-updater-exp-time : ui32 (default = 10);
    fml-updater-wait-on-error-time : ui32 (default = 5);
    data-updater-dir : string;
    data-updater-exp-time : ui32 (default = 10);
    data-updater-wait-on-error-time : ui32 (default = 5);
    areas : string;
    def-fml : TFormula;
    event-log : string;
    features-log : string;
    updater-log : string;
    preselection-log : string;
    feature-medians : {string -> string};
    wizard-url-mapping-files : [string];
    test-source-data : string;
    geodb-proto-data : string;
    candidate-storage (cppname = CandidateStorageConfig) : TCandidateStorageConfig;
    counters (cppname = CountersConfig) : TCountersConfig;
    external-bandit-vars-file : string;
    sources : {string -> TSourceConfig};
    fetch-params : {string -> string};
    pure-config : string;
    region-resolver (cppname = RegionResolverConfig) : TRegionResolverConfig;
    distr-report : string (default = "");
    mobile-goals-dict : string (default = "");
    use-local-ws-calc : bool (default = false);
    verbose : bool (default = false);
    type : string;
    apphost (cppname = AppHostConfig) : TAppHostConfig;
    strict : bool (default = false);
    response-time-stats : TStatsConfig;
    init-time-stats : TStatsConfig;
    candidate-stats : TStatsConfig;
    shown-candidate-stats : TStatsConfig;
    degradation-formula : string (default = "basic");
    degradation-config : TDegradationConfig;
    user-data-storage-config : TUserDataStorageConfig;
    default-filters-path : string;
    quota-file : string;
    protocol-options : {string -> string};
    experiment-config : TExperimentConfig;
    rtmr-setup-config : TRtmrSetupConfig;
    exclude-from-joined-product-profiles : [string];
    pq-config : TPqConfig;
    disable-overload-check : bool (default = false);
};

struct TWizardInfo {
    name : string;
    index : ui32;
};

struct TSourceResponse {
    error : string;
    time : ui64;
};

struct TFeatureAnalysis {
    idx : ui32;
    val : double;
    median : double;
    name : string;
    impact : double;
};

struct TUncompressedFeature {
    idx : ui16;
    name : string;
    description : string;
    value : double;
};

struct TBetaParams {
    alpha : ui64;
    beta : ui64;
};

struct TChoice {
    u : string;
    s : double;
    ws : double;
    p : double;
    w : double;
    tt : string;
    ts : string;
    path : string;
    aux : {string -> string};
    features : string;
    uncompressed-features : [TUncompressedFeature];
    boost-features : string;
    uncompressed-boost-features : [TUncompressedFeature];
    beta-params : TBetaParams;
};

struct TCandidateList {
    name : string;
    urls : [TCandidate];
};

struct TAuxInfo {
    candidate-infos : [TChoice];
    props : any;
};

struct TFormulaBucket {
    right-border : double;
    count : ui64;
};

struct TFormulaValues {
    min : double;
    buckets : [TFormulaBucket];
};

struct TFormulaDump {
    pass-early-stages-probability : double;
    pass-later-stages-probability : double;
    current-show-percentage : double;
    is-initialized : bool;
    median : ui64;
    non-empty-buckets : ui32;
    bucket-count : ui64;
    discounted-value-count : ui64;
    real-value-count : ui64;
    values : TFormulaValues;
};

struct TStorageDump {
    fml-count : ui32;
    formula-dumps : {string -> TFormulaDump};
};

struct TExperimentResponse {
    formula-thresholds : {string -> double};
    storage-dump : TStorageDump;
};

struct TSmartResponse {
    name : string;
    message : string;
    bandits-debug : {string -> [ui64]};
    sources : {string -> TSourceResponse};
    error : string;
    warnings : [string];
    boost-fml : string;
    boost : string;
    feature-filter-result : string;
    filtering-results : [string];
    formula-name : string;
    formula-link : string;
    success : bool;
    choice : TChoice;
    feature-analysis : {string -> [TFeatureAnalysis]};
    urls : [TChoice];
    candidates : [TCandidateList];
    groups : {string -> ui32};
    client-results : [TSmartResponse];
    collect-pool-mode : bool;
    optimized : bool;
    lr : ui32;
    aux-info : TAuxInfo;
    response-degraded : bool (default = false);
    local-user-data : string;
    used-show-percentage : bool (default = false);
};

///////////////////////// FRONT /////////////////////////

struct TFrontServerConfig {
    port : ui16;
    thread-count : ui16 (default = 0);
    max-queue-size : ui16 (default = 0);
    client-timeout : ui16 (default = 0);
    max-connections : ui16 (default = 100);
};

struct TFrontSrcConfig {
    backends : [string];
    balancing-scheme: string (default = "weighted");
    l2-backends : [string];
    l2-balancing-scheme : string (default = "weighted");
    path : string;
    connect-timeout : ui32;
    connect-retry-count : ui16;
    timeout : ui32;
    retry-count : ui16;
};

struct TFrontRuntimeConfig {
    path : string;
    updater-interval : ui32 (default = 10);
};

struct TFrontServiceConfig {
    worker-count : ui16;
    timeout : ui32 (default = 150);
    context-dict : string;
    uatraits-dict (cppname = UaTraitsDict) : string;
    shared-files-path : string;
    apphost : TFrontSrcConfig;
    reqans-log : string;
    event-log : string;
    req-log : string;
    main-log : string;
    param-alias : {string -> string};
    runtime-config : TFrontRuntimeConfig;
    time-stats : TStatsConfig;
    candidate-stats : TStatsConfig;
    fml-stats : TStatsConfig;
    max-testids (cppname = MaxTestIdsInUnistat): ui16 (default = 45);
};

struct TPromoLibRequest {
    uuid : string;
    locale : string;
    screen_height : ui16;
    screen_width : ui16;
    app_id : string;
    app_platform : string;
    app_version : string;
    app_version_name : string;
    region_id : ui32;
    device_type : string;
    manufacturer : string;
    os_version : string;
    model : string;
};

struct TTrainingSampleConfig {
    priority : ui32 (default = 0);
    ratio : double;
    filter : string;
};

struct TFrontRequest {
    rr : TSmartRequest;
    rerank-aux : string;

    fml : string;
    candidates : [TCandidate];
    yandexuid : string;
    i : string;
    uuid : string;
    login : string;
    loginhash : string;
    force-reqid : string;
    client-reqid : string;
    request-filter : string;
    dbg : bool (default = false);
    pretty : bool (default = false);
    position-boost : string;
    position-boost-weight : double;

    log-answers-ratio : double (default = 1.);
    dumpfeat-ratio : double (default = 0.);
    training-sample : {string -> TTrainingSampleConfig};

    descfeat : bool (default = false);
    dumpreq : bool (default = false);
    dumpresp : bool (default = false);
    dumpctx : bool (default = false);

    yabs-format : bool (default = false);

    empty-interval : ui32;
    display-interval : ui32;
    display-per-session : ui32;

    dump-props : [string];
};

struct TConditionalParams {
    condition : {string -> string};
    set : {string -> string};
};

struct TPresetParams {
    type : string;
    timeout : ui32;
    apphost : TFrontSrcConfig;
    protocol : string (default = "atom");
    disabled : bool (default = false);
    pos : ui16 (default = 0);
    proto : string (default = "GLOBAL");
    params : TFrontRequest;
    auto-params : {string -> TConditionalParams};
    custom-params : {string -> string};
    sub : {string -> TPresetParams};
};

struct TFrontConfig {
    server : TFrontServerConfig;
    service : TFrontServiceConfig;
    clients : {string -> TPresetParams};
};

struct TDoc {
    banner_id : string;
    link : string;
    score : double;
    weighted-score : double;
    initial-pos : ui16;

    title : string;
    snippet : [string];
    source-aux : {string -> string};

    persfeat : [double];
    boostfeat : [double];
    featuredump : [double];
    boost-featuredump : [double];

    persfeat-info : [TUncompressedFeature];
    boostfeat-info : [TUncompressedFeature];

    beta-params : TBetaParams;
};

struct TConvertedAuxInfo {
    candidate-infos : [TDoc];
    props : any;
};

struct TFrontResponse {
    apphost-request : [TAppHostSmartRequestCtx];
    apphost-response : any;
    rerankd-response : TSmartResponse;

    name : string;
    reqid (cppname = ReqID) : string;
    message : string;
    error : string;
    apphost-error : any;
    warnings : [string];

    formula-name : string;
    formula-link : string;

    sources : {string -> TSourceResponse};
    boost : string;
    boost-fml : string;
    rerank-success : bool;
    collect-pool-mode : bool;

    docs : [TDoc];
    docs-unranked : [TDoc];
    client-results : [TFrontResponse];
    aux-info : TConvertedAuxInfo;
    filtering-results : [string];
    debug-info : any;
    banner : struct {
        data : struct {
            stripe_universal : {string -> string};
        };
    };
};
