LIBRARY()

OWNER(
    muzich
    g:ydo
)

SRCS(
    GLOBAL factor_names.cpp
)

PEERDIR(
    kernel/factor_storage
    kernel/generated_factors_info
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NYdo
    NBegemotFactors
)

GENERATE_ENUM_SERIALIZATION(factors_gen.h)

END()
