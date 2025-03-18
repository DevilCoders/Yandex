namespace NImages;

struct TImgMergeTagsUpperScheme {
    Enabled : bool (default = true);
    TagsDisplayThreshold : double (default = 1.0);
    TagsInserterEnabled : bool (default = true);
    TagsInserterVersion : string (default = "NeuralVersion1"); // HeuristicVersion1 or NeuralVersion1
    AutoTagsInsertThreshold : double (default = 0.5);
};
