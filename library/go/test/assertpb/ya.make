GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(assert.go)

GO_TEST_SRCS(assert_test.go)

END()

RECURSE(gotest)
