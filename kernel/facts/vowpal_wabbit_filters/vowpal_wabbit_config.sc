namespace NVwFilters;

struct TVwFilterConfig {
    "enabled"    : bool (required);
    "fml"        : string (required);
    "threshold"  : double (required);
    "ngram"      : ui32 (required);
    "namespaces" : [string] (required);

    (validate) {
        TString error = helper->ValidateNamespaces(Namespaces());
        if (error) {
            AddError("\"namespaces\" validation failed: " + error);
        }
    };
};

struct TVwMultiFilterConfig {
    "filters" : { string -> TVwFilterConfig } (required);
};
