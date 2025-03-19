UNITTEST()

OWNER(
    kcd
    g:base
)

DATA(sbr://2170671437)

SRCS(
    detect_index_format_ut.cpp
    index_format_usage_ut.cpp
)

PEERDIR(
    kernel/doom/info
    library/cpp/testing/unittest
)

END()
