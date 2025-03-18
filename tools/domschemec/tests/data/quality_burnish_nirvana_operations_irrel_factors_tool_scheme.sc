namespace NBurnish;

struct TIrrelFactors {
    IrrelVerdicts (required) : ui64;
    TotalVerdicts (required) : ui64;
    Toloka404 : ui64 (default = 0);
    Tellurium404 : ui64 (default = 0);
};

struct TUrlFactors {
    IrrelFactors : TIrrelFactors;
};

struct TQueryInfo {
    Query (required): string;
    Region (required): i32;
    UrlFactors (required): {string -> TUrlFactors};
};

