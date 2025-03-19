PROTO_LIBRARY()

OWNER(g:base)

SRCS(
    clustering.proto
)

PEERDIR(
    mapreduce/yt/interface/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()

