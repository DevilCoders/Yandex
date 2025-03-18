namespace NRandomPorno;

struct TParentConfig {
    PermutationsPath : string(default = "permutations.json");
    Query2PermutationPath : string(default = "queries2perm.json");
};

struct TContext {
    Enabled : bool (default = false);
    Expression : string (default = "");
    Mode : string(default = "all");
    Salt : ui32(default = 360);
    TopToMark : ui32 (default = 10);
};

struct TParentRoot {
    Permutations : [[ui32]];
    Queries : {string -> [ui32]};
};
