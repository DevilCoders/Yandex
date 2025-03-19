#pragma once

namespace NCS {
    namespace NScheme {
        enum class ESchemeFormat {
            Default /* "default" */,
            OpenApi /* "openapi" */,
        };

        enum class EElementType {
            String /* "string" */,
            Text /* "text" */,
            Numeric /* "numeric" */,
            Boolean /* "bool" */,
            Structure /* "structure" */,
            Array /* "array_types" */,
            Variants /* "variants" */,
            MultiVariants /* "multi_variants" */,
            WideVariants /* "wide_variants" */,
            Json /* "json" */,
            Map /* "map" */,
            Ignore /* "ignore" */
        };

        enum class ERequestMethod {
            Get /* "get" */,
            Post /* "post" */,
            Put /* "put" */,
            Delete /* "delete" */
        };
    }
}
