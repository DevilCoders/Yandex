namespace NVideoPersdata;

struct TCommonOptions {
    Cluster: string;
    LogLevel: string (default="info");
    Owners: string[];
    ErrorLog: string;
};

struct TWord2VecModel {
    Name: string;
    Type: string; // "chunked", "yandex"
};

struct TModelsConfig {
    YTPrefix: string;
    LocalPath: string;
    DistributedPath: string;
    W2VModels: TWord2VecModel[];
};

struct TNeedsModels {
    ModelsConfig: TModelsConfig;
};

struct TDatasetConfig: TNeedsModels {
    PredictionLag: ui32;
    MinLearningItems: ui32;
    MaxLearningItems: ui32;
    LearningItemsPerUser: ui32;
    UserSelectionProbability: double;
    ProfileType (required): string;

    CalculateQueryVectors: bool (default=false);

    ChainsTable (required): string;
    UrlDataTable (required): string;

    IntermediateOutputPath (required): string;
    ClustersTable (required): string;

    PoolOutputPath (required): string;
};

struct TProfileConfig: TNeedsModels {
    ProfileSize (required): ui32;
    MinProfileSize (required): ui32;
    MinProfileFreshness: ui32;
    ItemTypes (required): string[];

    IntermediatePath (required): string;
    ProfileOutputPath (required): string;
};

struct TRecommendationsConfig: TNeedsModels {
    RecommendUrls: ui32 (default=30);
    Candidates: ui32 (default=2000); // number of candidate urls to consider
    LimitProfiles: ui32 (default=0); // 0 means all profiles
    ProfileSize: ui32 (default=0); // 0 means all items

    ProfilePath (required): string;
    CandidateUrls (required): string;
    FilterUids: string;
    FilterUrls: string;
    RecommendationsOutputPath (required): string;
};

struct TTestConfig: TNeedsModels {
    Text: string;
};

struct TDebugRecommendationsConfig {
    ProfilePath (required): string;
    RecommendationsPath (required): string;
    DebugTablesOutputPath (required): string;
};

struct TSaasExportConfig {
    RecommendationsPath: string;
    SaasExportPath: string;
    PortionLifetime: ui64;
};

struct TPersdataConfig {
    CommonOptions (required): TCommonOptions;

    DatasetConfig: TDatasetConfig;
    ProfileConfig: TProfileConfig;
    RecommendationsConfig: TRecommendationsConfig;
    TestConfig: TTestConfig;
    DebugRecommendationsConfig: TDebugRecommendationsConfig;
    SaasExportConfig: TSaasExportConfig;
};
