GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    errors.go
    marketplace.go
)

GO_TEST_SRCS(
    marketplace_test.go
)

END()

RECURSE(
    gotest
    mocks
)
