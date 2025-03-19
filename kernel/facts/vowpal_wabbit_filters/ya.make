LIBRARY()

OWNER(
    g:shinyserp
    geomslayer
)

SRCS(
    vowpal_wabbit_filters.cpp
    vowpal_wabbit_config.sc
)

PEERDIR(
    library/cpp/scheme
    library/cpp/vowpalwabbit
)

END()

RECURSE(
    convert_model_tool
    pylib
)

RECURSE_FOR_TESTS(
    ut
    ut/data
)
