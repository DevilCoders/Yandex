LIBRARY()

OWNER(
    dmitryno
    g:wizard
    gotmanov
)

PEERDIR(
    contrib/libs/protobuf
    contrib/libs/protoc
    kernel/gazetteer/common
    kernel/gazetteer/proto
)

SRCS(
    builtin.cpp
    descriptors.cpp
    protoparser.cpp
    protopool.cpp
    sourcetree.cpp
    version.cpp
)

END()
