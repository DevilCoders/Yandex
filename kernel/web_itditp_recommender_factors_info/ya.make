LIBRARY()

LICENSE(YandexNDA)

OWNER(
    g:itditp
    shotinleg
)

SRCS(
    factor_names.cpp
    GLOBAL ${BINDIR}/factors_gen.cpp
)

PEERDIR(
    kernel/factors_info
    kernel/factor_slices
    kernel/factor_storage
    kernel/generated_factors_info
)

SPLIT_CODEGEN(kernel/generated_factors_info/factors_codegen factors_gen NWebItdItpRecommender)

END()
