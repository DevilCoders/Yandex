LIBRARY()

LICENSE(YandexNDA)

OWNER(
    ilnurkh
    g:base
    g:factordev
)

SRCS(
    GLOBAL factor_names.cpp
)

IF (GCC)
    CFLAGS(-fno-var-tracking-assignments)
ENDIF()

PEERDIR(
    kernel/factors_info
    kernel/factor_slices
    kernel/factor_storage
    kernel/generated_factors_info
)

SPLIT_CODEGEN(
    OUT_NUM 7
    kernel/generated_factors_info/factors_codegen factors_gen
)

END()

RECURSE_FOR_TESTS(
    ut
    all_web_slices_checks
)
