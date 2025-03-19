OWNER(ilnurkh)

PROTO_LIBRARY()

SRCS(
    features_remap.proto
)

GENERATE_ENUM_SERIALIZATION(features_remap.pb.h)

EXCLUDE_TAGS(GO_PROTO)

END()
