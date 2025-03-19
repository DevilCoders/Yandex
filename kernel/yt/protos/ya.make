PROTO_LIBRARY()

OWNER(
    g:fastcrawl
    g:jupiter
)

SRCS(
    extension.proto
    perfstats.proto
)

PEERDIR(
    mapreduce/yt/interface/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
