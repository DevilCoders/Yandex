LIBRARY()

OWNER(
    shotinleg
    g:base
)

PEERDIR(
    kernel/country_data
    kernel/factor_storage
    kernel/web_itditp_factors_info
    kernel/web_factors_info

    library/cpp/langs

    ysite/yandex/erf_format
    ysite/yandex/relevance
)

SRCS(
    fill_web_itditp_left_static_features.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
