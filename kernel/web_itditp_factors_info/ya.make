LIBRARY()

LICENSE(YandexNDA)

OWNER(
    g:base
)

PEERDIR(
    kernel/factors_info
    kernel/factor_slices
    kernel/factor_storage
    kernel/generated_factors_info
)

SRCS(
    factor_names.cpp
    GLOBAL ${BINDIR}/factors_gen.cpp
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NWebItdItp
)

END()
