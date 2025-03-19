PROTO_LIBRARY()

OWNER(
    g:blender
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    learning_metrics.proto
    model.proto
    request_data.proto
    train_params.proto
    util.proto
)
END()
