LIBRARY()

OWNER(
    nsstepanov
    g:base
)

PEERDIR(
    kernel/factor_storage
    kernel/web_itditp_factors_info
    kernel/web_factors_info

    search/reqparam

    ysite/yandex/relevance
)

SRCS(
    fill_web_itditp_platform_features.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()