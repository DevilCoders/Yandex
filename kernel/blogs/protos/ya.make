PROTO_LIBRARY()

OWNER(sashateh)

SRCS(
    blogs.proto
    rss.proto
)

IF (NOT PY_PROTOS_FOR)
    EXCLUDE_TAGS(GO_PROTO)
ENDIF()

END()
