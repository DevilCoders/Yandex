PROTO_LIBRARY()

OWNER(
    g:indexann
)

SRCS(
    data.proto
    portion.proto
)

IF (NOT PY_PROTOS_FOR)
    EXCLUDE_TAGS(GO_PROTO)
ENDIF()

END()
