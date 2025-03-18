namespace NImages;

struct TImgFilterSimilarScheme {
    Enabled : bool (default = true);
    PornoThreshold : double (default = 0.95);
    VwPornoThreshold : double (default = 0.8);
    CpThreshold : double (default = 0.94);
    GruesomeThreshold : double (default = 1.0);

    PornoAvgThreshold : double (default = 0.75);
    VwPornoAvgThreshold : double (default = 1.0);
    CpAvgThreshold : double (default = 0.75);
    GruesomeAvgThreshold : double (default = 0.75);

    AveragePart : ui32 (default = 20);
};
