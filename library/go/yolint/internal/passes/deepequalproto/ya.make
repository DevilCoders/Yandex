GO_LIBRARY()

OWNER(
    g:go-library
    prime
)

SRCS(deepequalproto.go)

GO_TEST_SRCS(deepequalproto_test.go)

END()

RECURSE(gotest)
