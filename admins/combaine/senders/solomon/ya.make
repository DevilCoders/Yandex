GO_LIBRARY()

OWNER(g:music-sre)

SRCS(
    config.go
    grpc.go
    send.go
    task.go
)

END()

RECURSE(
    cmd
)
