LIBRARY()

OWNER(
    vvp
    g:base
)

SRCS(
    static.fml
    calc_static.cpp
    calc_regstatic.cpp
    static_factors.inc.sfdl
)

PEERDIR(
    kernel/externalrelev
    kernel/factor_storage
    kernel/index_mapping
    kernel/region2country
    kernel/relevfml
    kernel/remap
    kernel/search_types
    kernel/web_factors_info
    library/cpp/deprecated/dater_old
    library/cpp/on_disk/head_ar
    ysite/yandex/dates
    ysite/yandex/erf
    ysite/yandex/erf_format
    ysite/yandex/relevance
)

END()
