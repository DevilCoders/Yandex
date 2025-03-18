namespace NImages;

struct TImgCommercialTitlesScheme {
    Enabled : bool (default = true);
    MaxNumDocumentsToProcess : ui16 (default = 100);
    MaxNumTitleToReturn : ui16 (default = 20);
    SigmoidAlpha : double (default = -15.0);
    SigmoidShift : double (default = -0.4);
    Threshold : double (default = 0.378);
};
