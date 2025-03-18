#pragma once

enum class EQueriesFormat {
    Binary /* "binary" */,
    Tsv /* "tsv" */,
};

enum class EDistance {
    L1Distance /* "l1_distance" */,
    L2SqrDistance /* "l2_sqr_distance" */,
    DotProduct /* "dot_product" */,
    Unknown /* "unknown" */,
};

enum class EVectorComponentType {
    I8 /* "i8" */,
    I32 /* "i32" */,
    Float /* "float" */,
    Unknown /* "unknown" */,
};
