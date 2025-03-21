LIBRARY()

GENERATE_ENUM_SERIALIZATION(node.h)

PEERDIR(
    library/cpp/yson
    library/cpp/yson/json
)

OWNER(
    ermolovd
    g:yt
)

SRCS(
    node.cpp
    node_io.cpp
    node_builder.cpp
    node_visitor.cpp
    serialize.cpp
)

END()

RECURSE(
    benchmark
    ut
)
