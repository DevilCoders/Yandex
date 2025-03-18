GO_LIBRARY()

OWNER(g:go-library)

SRCS(protoseq.go)

GO_TEST_SRCS(protoseq_test.go)

END()

RECURSE(
    gotest
    internal
)
