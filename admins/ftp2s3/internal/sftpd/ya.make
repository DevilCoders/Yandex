GO_LIBRARY()

OWNER(
    skacheev
    g:music-sre
)

SRCS(
    conn.go
    flags.go
    handler.go
    io.go
    server.go
)

GO_TEST_SRCS(server_test.go)

END()

RECURSE(gotest)
