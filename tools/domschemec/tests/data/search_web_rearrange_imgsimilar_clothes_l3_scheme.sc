namespace NImages;

struct TImgSimilarClothesL3Scheme {
    Enabled : bool      (default = false);
    UseL3Formula : bool (default = true);
    RerangeTop : ui32   (default = 50);
    GrepFactors : bool   (default = false);
    ModelName : string   (default = "");
};
