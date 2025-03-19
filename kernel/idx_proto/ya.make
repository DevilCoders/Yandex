PROTO_LIBRARY()

OWNER(
    g:base
    gotmanov
    apos
    solozobov
)

SRCS(
    feature_pool.proto
    meta_prs_ops.proto
)

PEERDIR(
    kernel/indexdoc/entry_protos
    kernel/tarc/protos
    kernel/text_machine/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
