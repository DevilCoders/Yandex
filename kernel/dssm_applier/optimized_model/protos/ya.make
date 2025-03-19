PROTO_LIBRARY()

OWNER(
    filmih
    g:neural-search
)

PEERDIR(
    kernel/dssm_applier/embeddings_transfer/proto
    kernel/factors_selector/proto
)

SRCS(
    apply_result.proto
    bundle_config.proto
)

GENERATE_ENUM_SERIALIZATION(bundle_config.pb.h)

EXCLUDE_TAGS(GO_PROTO)

END()
