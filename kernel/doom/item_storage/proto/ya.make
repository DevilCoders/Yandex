PROTO_LIBRARY()

OWNER(
    sskvor
    g:base
)

SRCS(
    status.proto
    status_code.proto
)

GENERATE_ENUM_SERIALIZATION(status_code.pb.h)

EXCLUDE_TAGS(GO_PROTO)

END()
