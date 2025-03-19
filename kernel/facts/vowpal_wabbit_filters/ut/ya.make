UNITTEST()

OWNER(
    g:shinyserp
    geomslayer
)

SRCS(
    config_validation_ut.cpp
    query_data_ut.cpp
    vowpal_wabbit_filters_ut.cpp
)

PEERDIR(
    kernel/facts/vowpal_wabbit_filters
)

DEPENDS(
    kernel/facts/vowpal_wabbit_filters/ut/data
)

DATA(
    arcadia/kernel/facts/vowpal_wabbit_filters/ut
)

END()
