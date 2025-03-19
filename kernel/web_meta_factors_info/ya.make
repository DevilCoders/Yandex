LIBRARY()

LICENSE(YandexNDA)

OWNER(
    ibelotelov
    alsafr
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

SRCS(
)

SPLIT_CODEGEN(
    OUT_NUM 6
    kernel/generated_factors_info/factors_codegen factors_gen NWebMeta
)

SPLIT_CODEGEN(
    OUT_NUM 6
    kernel/generated_factors_info/factors_codegen rearrs_factors_gen NWebMetaRearrs
)

END()

