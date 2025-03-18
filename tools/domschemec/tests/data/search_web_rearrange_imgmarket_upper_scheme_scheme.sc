namespace NImages;

struct TImgMarketScheme {
    Enabled : bool (default = true);
    MarketGroupsOnly : bool (default = false);
    CopRelevanceScale : double (default = 1.0);
    SimRelevanceScale : double (default = 0.00000000023283064); // 1 / Max<ui32>()
    ImagesFromCbir : ui16 (default = 10);
    StablePageSize : ui16 (default = 20);
    CbirFirst : bool (default = true);
    ClearMarketGroups : bool (default = true);
    AdditionalCGI : string (default = "snip=exps=similarmarket");
    RerangByI2T : bool (default = true);
    AddSimilarOffers : bool (default = false);
    ResultFilter : bool (default = false);
    MinRelevance : double (default = 0.312551);
    MinRateRelevance : double (default = 0.36284344);
    MinI2TRelevance : double (default = 0.0);
};
