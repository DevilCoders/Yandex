OWNER(
    petrk
    yulika
    yurakura
)

LIBRARY()

SRCS(
    bestgeo.cpp
    entity.cpp
    geodb.cpp
    util.cpp
)

PEERDIR(
    contrib/libs/protobuf
    kernel/geodb/protos
    kernel/search_types
    library/cpp/containers/dense_hash
    library/cpp/langs
    library/cpp/streams/factory
)

END()
