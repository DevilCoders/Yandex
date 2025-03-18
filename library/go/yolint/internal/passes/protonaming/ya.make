GO_LIBRARY()

OWNER(
    g:go-library
    prime
)

SRCS(protonaming.go)

GO_TEST_SRCS(protonaming_test.go)

END()

RECURSE(gotest)
