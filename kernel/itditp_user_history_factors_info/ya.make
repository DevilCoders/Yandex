LIBRARY()

LICENSE(YandexNDA)

OWNER(
    ulyanin
    sankear
    yustuken
)

SRCS(
    factor_names.cpp
    GLOBAL ${BINDIR}/factors_gen.cpp
)

PEERDIR(
    kernel/generated_factors_info
)

SPLIT_CODEGEN(kernel/generated_factors_info/factors_codegen factors_gen NItdItpUserHistory)

END()
