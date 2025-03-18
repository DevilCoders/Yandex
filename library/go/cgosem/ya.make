GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(sem.go)

GO_TEST_SRCS(leak_test.go)

END()

RECURSE(
    dummy
    gotest
)
