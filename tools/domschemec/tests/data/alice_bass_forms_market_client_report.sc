namespace NBassApi;

struct TFormalizedGlFilter {
    type: string (required, allowed = ["boolean", "number", "enum"]);
    values: [struct {
        id: ui64 (required);
        num: double (required);
    }];
};

struct TFormalizedGlFilters {
    filters: { ui64 -> TFormalizedGlFilter } (required);
};