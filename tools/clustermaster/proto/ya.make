PROTO_LIBRARY()

OWNER(
    g:clustermaster
    g:jupiter
    g:fastcrawl
)

SRCS(
    master_options.proto
    messages.proto
    solver_options.proto
    state_row.proto
    target.proto
    variables.proto
    worker_options.proto
)

PEERDIR(
    library/cpp/getoptpb/proto
    mapreduce/yt/interface/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
