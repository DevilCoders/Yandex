LIBRARY()

OWNER(
    crossby
    g:factordev
)

PEERDIR(
    kernel/prs_log/data_types
    kernel/prs_log/serializer/factor_storage
    kernel/prs_log/serializer/uniform_bound
    library/cpp/streams/lz
)

SRCS(
    serialize.cpp
)

GENERATE_ENUM_SERIALIZATION(serialize.h)

END()

RECURSE(
    ut
)
