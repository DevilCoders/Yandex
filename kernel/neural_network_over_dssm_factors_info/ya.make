LIBRARY()

LICENSE(YandexNDA)

OWNER(
    g:factordev
    g:base
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
    NNeuralNetworkOverDssmFactors
)

END()
