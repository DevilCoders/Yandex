LIBRARY()

OWNER(
    g:fastcrawl
    g:jupiter
)

SRCS(
    fwd.cpp
    client.cpp
    schema.cpp
    table.cpp
    timestamp.cpp
    proto_api.cpp
)

ADDINCL(yt)

PEERDIR(
    contrib/libs/protobuf
    library/cpp/getopt/small
    library/cpp/protobuf/util
    library/cpp/threading/skip_list
    library/cpp/yson
    mapreduce/yt/interface/protos
    kernel/yt/protos
    kernel/yt/utils
    yt/yt/core
    yt/yt/client
)

GENERATE_ENUM_SERIALIZATION(timestamp.h)

END()
