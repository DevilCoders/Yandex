GO_LIBRARY()

OWNER(
    djerys
    sidh
    g:go-library
)

SRCS(
    error.go
)

GO_TEST_SRCS(
    error_test.go
)

END()

RECURSE(
    gotest
)
