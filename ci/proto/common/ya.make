OWNER(g:ci)
PROTO_LIBRARY(ci-proto-common)
EXCLUDE_TAGS(GO_PROTO)

GRPC()

SRCS(
    info_panel.proto
)


END()
