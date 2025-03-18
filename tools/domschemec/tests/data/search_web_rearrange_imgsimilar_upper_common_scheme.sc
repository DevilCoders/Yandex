namespace NImages;

struct TImgSimilarScheme {
    Enabled : bool (default = true);
    AddBestImg : bool (default = true);
    AddImgResult : bool (default = false);
    AddCbirResult : bool (default = false);
    AddSimResult : bool (default = true);
    RemoveImgDocs : bool (default = true);
    RemoveCbirDocs : bool (default = true);
    RemoveSimilarDocs : bool (default = false);

    AddSimilarPornResult : bool (default = true);
    SimilarBinaryPornMinThreshold : double (default = 0.50);
    SimilarPornWithEroticMinThreshold : double (default = 0.43);
    SimilarDocPornThreshold : double (default = 0.25);
    SimilarBinaryPornHardThreshold : double (default = 1.0);
    SimilarPornWithEroticHardThreshold : double (default = 1.0);
};
