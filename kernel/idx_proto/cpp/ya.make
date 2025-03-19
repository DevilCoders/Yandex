LIBRARY()

OWNER(
    g:base
    gotmanov
    apos
    solozobov
)

SRCS(
    feature_pool.cpp
    format.cpp
    grouping.cpp
    utils.cpp
)

PEERDIR(
    kernel/idx_ops
    kernel/idx_proto
)

GENERATE_ENUM_SERIALIZATION(grouping.h)

END()
