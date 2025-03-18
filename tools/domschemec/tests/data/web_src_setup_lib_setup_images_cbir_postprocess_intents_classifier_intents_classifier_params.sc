namespace NImages;

struct TCbirIntentsScheme {

    struct TCbirIntentsManual {
        QRCodeTypes : string[]              (default = ["QRCode"]);
        QRCodeW : double                    (default = 0.9);
        QRCodeLow : double                  (default = 0.19);

        EntityTagSearchThreshold : double   (default = 0.85);
        EntityI2TDistanceThreshold : double (default = 0.287);
        EntityW : double                    (default = 0.8);
        EntityLow : double                  (default = 0.18);

        NotClothesCathegory : ui16[]        (default = [23]);
        SkipClothesCathegory : ui16[]       (default = [20, 21]);
        ClothesFaceAspect : double          (default = 0.8);
        ClothesMinArea : double             (default = 0.1);
        ClothesFaceMaxArea : double         (default = 0.1);
        ClothesMinObjectness : double       (default = 0.1);
        ClothesMinPrWithFace : double       (default = 0.05);
        ClothesMinPr : double               (default = 0.2);
        ClothesW : double                   (default = 0.7);
        ClothesLow : double                 (default = 0.17);

        MarketThreshold : double            (default = 0.3);
        MarketW : double                    (default = 0.6);
        MarketLow : double                  (default = 0.16);

        TagsHighThreshold : double          (default = 0.85);
        TagsMedThreshold : double           (default = 0.78);
        TagsLowThreshold : double           (default = 0.67);
        TagsTinyThreshold : double          (default = 0.5);
        TagsHW : double                     (default = 0.5);
        TagsMW : double                     (default = 0.5);
        TagsLW : double                     (default = 0.5);
        TagsLow : double                    (default = 0.15);

        OcrMinLines : double                (default = 0.75);
        OcrMinWords :  double               (default = 0.88);
        OcrW : double                       (default = 0.4);
        OcrLow : double                     (default = 0.14);

        FaceLayers : string[]               (default = ["faces_v4_fc_encoder_256", "faces_v3_fc_encoder"]);
        FaceMinArea : double                (default = 0.01);
        FaceThreshold : double              (default = 0.33);
        FaceW : double                      (default = 0.3);
        FaceLow : double                    (default = 0.13);
        PeopleW : double                    (default = 0.35);
        PeopleLow : double                  (default = 0.135);

        SimilarW : double (default = 0.2);
    };

    ManualBoosting : TCbirIntentsManual;
};
