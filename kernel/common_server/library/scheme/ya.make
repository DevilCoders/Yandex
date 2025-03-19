LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/scheme/proto
    kernel/common_server/library/scheme/serialization
    kernel/common_server/util
    library/cpp/logger/global
    library/cpp/object_factory
    library/cpp/json/writer
    library/cpp/protobuf/json
    contrib/libs/protobuf
)

SRCS(
    abstract.cpp
    fields.cpp
    common.cpp
    scheme.cpp
    operations.cpp
    handler.cpp
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(common.h)
GENERATE_ENUM_SERIALIZATION_WITH_HEADER(abstract.h)
GENERATE_ENUM_SERIALIZATION_WITH_HEADER(fields.h)
GENERATE_ENUM_SERIALIZATION_WITH_HEADER(scheme.h)

END()
