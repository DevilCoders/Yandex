LIBRARY()

LICENSE(YandexNDA)

OWNER(
    andreytert
)

SRCS(
    GLOBAL factor_names.cpp
)

PEERDIR(
    kernel/factors_info
    kernel/factor_storage
    kernel/generated_factors_info
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NSelectionRank
    NFactors)

END()
