UNITTEST()

OWNER(gotmanov)

PEERDIR(
    ADDINCL kernel/web_factors_info
    kernel/web_factors_info/validators
    kernel/web_meta_factors_info
    search/lingboost/production
)

SRCDIR(kernel/web_factors_info)

SRCS(
    web_factors_info_ut.cpp
)

END()
