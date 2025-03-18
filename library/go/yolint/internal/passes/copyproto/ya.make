GO_LIBRARY()

OWNER(
    g:go-library
    prime
)

SRCS(copyproto.go)

GO_TEST_SRCS(copyproto_test.go)

END()

RECURSE(gotest)
