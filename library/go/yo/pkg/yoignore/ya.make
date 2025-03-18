GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(yoignore.go)

GO_TEST_SRCS(yoignore_test.go)

END()

RECURSE(gotest)
