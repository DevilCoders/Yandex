LIBRARY()

LICENSE(YandexNDA)

OWNER(
    alejes
    hygge
    g:factordev
    g:webfresh
)

PEERDIR(
    kernel/generated_factors_info
)

SRCS(
    GLOBAL factor_names.cpp
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NBegemotQueryRtFactors
)

END()
