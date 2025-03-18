namespace NBlenderDssmFeaturesSc;

struct TModelWithMeta {
    Path (required) : string;
    Inputs (required): string[];
    DocEmbeddingOutput: string (default = "doc_embedding");
    QueryEmbeddingOutput: string (default = "query_embedding");
    ResultOutput: string (default = "joint_output");
    Intent: string;
    Timestamp: string;
    IsConveyor: bool;
};

struct TSerpDssmFeatureCalculator {
    Model (required) : string;
    FeatureIndex (required): ui32;
    VerticalName: string;
    VerticalDocCount: ui32;
    SerpTop: ui32;
    Enabled : bool (default = true);
    SurplusName : string;
};

struct TSerpDataOneItemExtractorArgs {
    Type (required) : string;
    UrlPath : string;
    TextPaths : string[];
    FilterPath : string;
    FilterValue : string;
    CutUrlParams : bool (default = false);
    ClearHilite : bool (default = false);
    UseDocUrl : bool (default = false);
    MaxTextLength: ui32 (default = 1000);
};

struct TSerpDataArrayExtractorArgs {
    Type (required) : string;
    BasePath (required) : string;
    Count (required) : ui32;
    UrlSubPath : string;
    TextSubPaths : string[];
    FilterPath : string;
    FilterValue : string;
    CutUrlParams : bool (default = false);
    ClearHilite : bool (default = false);
    MaxTextLength: ui32 (default = 1000);
};

struct TOrganicTextExtractorArgs {
    Type (required) : string;
    Count : ui32 (default = 1);
    CutUrlParams : bool (default = false);
    ClearHilite : bool (default = false);
    UseTitle : bool (default = true);
    UseSnippet : bool (default = false);
    MaxTextLength: ui32 (default = 1000);
};

