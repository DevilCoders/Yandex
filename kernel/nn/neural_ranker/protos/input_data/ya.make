PROTO_LIBRARY()

OWNER(g:factordev)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    input_data.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
