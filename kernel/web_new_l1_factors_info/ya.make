LIBRARY()

LICENSE(YandexNDA)

OWNER(
    nikita-uvarov
    g:base
    g:factordev
)

SRCS(
    factor_names.cpp
    GLOBAL ${BINDIR}/factors_gen.cpp
)

PEERDIR(
    kernel/web_factors_info
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NWebNewL1
)

END()
