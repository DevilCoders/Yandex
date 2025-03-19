OWNER(
    g:ugc
    stakanviski
)

PROTO_LIBRARY()

SRCS(
    api.proto
    schema.proto
    ugc.proto
)

IF (GEN_PROTO)
    RUN_PROGRAM(
        kernel/ugc/schema/compiler/bin --proto ${ARCADIA_ROOT}/kernel/ugc/schema/ugc.schema
        IN ${ARCADIA_ROOT}/kernel/ugc/schema/ugc.schema
        OUT_NOAUTO ugc.proto
        CWD ${ARCADIA_BUILD_ROOT}/kernel/ugc/schema/proto
    )
ENDIF()

EXCLUDE_TAGS(GO_PROTO)

END()
