namespace NImages;

struct TImgMergeTagsScheme {
    Enabled : bool (default = true);
    MergeDuckTags : bool (default = false);
    DuckTagsSaasTimeout : ui32 (default = 0); // Timeout for KV SaaS reply in ms. Zero means default timeout.
    TagsNumForI2TRearrange : ui32 (default = 10);
    MaxNumTagsPerDocument : ui32 (default = 10);
    FeaturesCompressionForMergeTagsEnabled : bool (default = true);
    RearrI2TVersion : string (default = "Auto"); // "Auto" means that I2T version will be selected in the order specified in the meta/rearrs/conf.json
    GeneralTagsI2TVersion : string (default = "Auto");

    BoostDuckTags : double (default = 1.5);
    DuckDepth : ui32 (default = 30);
    DuckModel : string (default = "ru");
    DuckTagsInDoc : ui32 (default = 1);
};
