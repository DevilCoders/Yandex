PROTO_LIBRARY()

OWNER(g:cloud-nbs)

INCLUDE_TAGS(GO_PROTO)

EXCLUDE_TAGS(PY_PROTO PY3_PROTO)

SRCS(
    auth.proto
    ic.proto
    log.proto
    sys.proto
)

END()
