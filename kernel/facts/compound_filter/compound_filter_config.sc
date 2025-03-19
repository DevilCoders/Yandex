namespace NFacts;

struct TCompoundFilterConfig {
    blacklist (required): struct {
        types: [
            string
        ];
        sources: [
            string
        ];
        substrings (required): [
            string
        ];
    };
    whitelist: struct {
        sources: [
            string
        ];
        hostnames: [
            string
        ];
        source_and_hostnames: [
            struct {
                source (required): string;
                hostname (required): string;
            }
        ];
    };
};
