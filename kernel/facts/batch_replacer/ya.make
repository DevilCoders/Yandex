LIBRARY()

OWNER(
    g:facts
)

GENERATE_ENUM_SERIALIZATION(batch_replacer_config.sc.h)

PEERDIR(
    contrib/libs/re2
    library/cpp/scheme
)

SRCS(
    batch_replacer.cpp
    batch_replacer_config.sc
)

END()

RECURSE_FOR_TESTS(
    ut
)
