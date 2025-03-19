LIBRARY()

OWNER(
    g:shinyserp
)

GENERATE_ENUM_SERIALIZATION(compound_filter_config.sc.h)

PEERDIR(
    library/cpp/scheme
)

SRCS(
    compound_filter.cpp
    compound_filter_config.sc
)

END()

RECURSE_FOR_TESTS(
    ut
)
