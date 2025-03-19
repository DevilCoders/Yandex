GO_LIBRARY()

OWNER(g:music-sre)

SRCS(
    logger.go
    serve.go
    types.go
)

END()

RECURSE(
    pb
    solomon
)
