GO_LIBRARY()

OWNER(
    sidh
    g:go-library
)

SRCS(
    cli.go
    kv.go
    tskv.go
)

GO_TEST_SRCS(
    kv_test.go
    tskv_test.go
)

END()

RECURSE(gotest)
