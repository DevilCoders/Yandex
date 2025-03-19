LIBRARY()

LICENSE(YandexNDA)

OWNER(
    g:factordev
)

PEERDIR(
    kernel/web_factors_info
)

SRCS(
    GLOBAL factor_names.cpp
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NWebProductionFormulaFeatures
)

END()
