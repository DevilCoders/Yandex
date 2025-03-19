PY2_LIBRARY()

OWNER(
    geomslayer
    g:shinyserp
)

PY_SRCS(
    __init__.py
    vowpal_wabbit_filter.pyx
)

PEERDIR(
    kernel/facts/vowpal_wabbit_filters
    library/cpp/scheme
)

END()
