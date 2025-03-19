PROTO_LIBRARY()

OWNER(
    nsofya
    osado
)

SRCS(
    archive_zone_attributes.proto
    snippets.proto
    tarc.proto
)

IF (NOT PY_PROTOS_FOR)
    EXCLUDE_TAGS(GO_PROTO)
ENDIF()

END()
