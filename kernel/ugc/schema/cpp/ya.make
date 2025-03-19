OWNER(
    g:ugc
    stakanviski
)

LIBRARY()

RUN_PROGRAM(
    kernel/ugc/schema/compiler/bin --cpp ${ARCADIA_ROOT}/kernel/ugc/schema/ugc.schema
    IN ${ARCADIA_ROOT}/kernel/ugc/schema/ugc.schema
    OUT ugc.schema.h
    OUTPUT_INCLUDES kernel/ugc/schema/cpp/schema-impl.h
    CWD ${ARCADIA_BUILD_ROOT}/kernel/ugc/schema/cpp
)

SRCS(
    build.cpp
)

PEERDIR(
    kernel/ugc/schema/proto
    library/cpp/protobuf/json
)

END()
