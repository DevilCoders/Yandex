namespace NDumperSc;

struct TRequestParamFilterArgs {
    Type (required) : string;
    Name (required) : string;
    Intent (required) : string;
    Values : string[];
    Prefixes : string[];
    Iznanka : bool (default = true);
    ClearUrlParam : bool (default = false);
};
