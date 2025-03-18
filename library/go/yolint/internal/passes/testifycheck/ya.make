GO_LIBRARY()

OWNER(
    g:go-library
    prime
)

SRCS(testifycheck.go)

GO_TEST_SRCS(testifycheck_test.go)

END()

RECURSE(gotest)
