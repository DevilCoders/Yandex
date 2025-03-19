// Documentation: https://wiki.yandex-team.ru/development/poisk/arcadia/tools/domscheme/

namespace NBatchReplacer;

struct TBatchReplacerConfig {
    "__root__" :  // Fake nameless level, just to comply with scheme-for-scheme rooles.
    {
        /*batch_name*/ string -> [
            struct {
                // No operation
                "nop": bool;

                // TShrinkSpacesOperation
                "shrink_spaces": bool;

                // TReplaceByPatternOperation
                "pattern": string;

                // TReplaceByRegexOperation
                "regex": string;

                // TReplaceByPatternOperation, TReplaceByRegexOperation
                "replacement": string;
                "global": bool;
                "repeated": bool;
            }
        ]
    };
};
