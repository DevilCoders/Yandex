GO_LIBRARY()

OWNER(g:go-library)

SRCS(
    multihost.go
    parse.go
)

GO_TEST_SRCS(
    multihost_test.go
    parse_test.go
)

END()

RECURSE(gotest)
