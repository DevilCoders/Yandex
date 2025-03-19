UNITTEST()

OWNER(
    lagrunge
    g:base
)

PEERDIR(
    ADDINCL kernel/factor_storage
    kernel/web_factors_info
)

SRCDIR(kernel/factor_storage)

SRCS(
    factor_storage_ut.cpp
    factors_reader_ut.cpp
    float_utils_ut.cpp
)

END()
