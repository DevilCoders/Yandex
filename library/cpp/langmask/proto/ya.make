PROTO_LIBRARY()

OWNER(mihaild)

SRCS(
    langmask.proto
)

IF (NOT PY_PROTOS_FOR)
    EXCLUDE_TAGS(GO_PROTO)
ENDIF()

END()
