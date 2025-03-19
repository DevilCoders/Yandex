LIBRARY()

LICENSE(YandexNDA)

OWNER(
    g:search-pers
    g:middle
    g:factordev
)

PEERDIR(
    kernel/factors_info
    kernel/factor_slices
    kernel/factor_storage
    kernel/generated_factors_info
)

SRCS(
    GLOBAL factor_names.cpp
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NWebMetaPers
)

END()
