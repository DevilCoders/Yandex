GO_LIBRARY()

OWNER(
    skacheev
    g:music-sre
)

SRCS(
    buf.go
    config.go
    disk_cache.go
    graphite.go
    sender.go
    status.go
)

GO_TEST_SRCS(buf_test.go)

END()

RECURSE(gotest)
