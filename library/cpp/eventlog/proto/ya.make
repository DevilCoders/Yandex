PROTO_LIBRARY()

OWNER(
    g:middle
    vmordovin
    pg
    mvel
)

IF (NOT PY_PROTOS_FOR)
    INCLUDE_TAGS(GO_PROTO)
ENDIF()

SRCS(
    events_extension.proto
    internal.proto
)

END()
