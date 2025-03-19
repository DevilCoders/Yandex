LIBRARY()

OWNER(
    desertfury
    g:base
)

PEERDIR(
    kernel/country_data
    kernel/factor_storage
    kernel/neural_network_over_dssm_factors_info
    kernel/generated_factors_info

    library/cpp/langs

    ysite/yandex/erf_format
    ysite/yandex/relevance
)

SRCS(
    fill_neural_network_over_dssm_factors.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
