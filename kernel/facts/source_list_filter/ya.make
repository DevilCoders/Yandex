LIBRARY()

OWNER(
    g:facts
)

GENERATE_ENUM_SERIALIZATION(source_list_filter_config.sc.h)

PEERDIR(
    library/cpp/scheme
    search/session
)

SRCS(
    source_list_filter.cpp
    source_list_filter_config.sc
)

END()

RECURSE_FOR_TESTS(
    ut
)
