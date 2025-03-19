GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    errors.go
    manager.go
    state.go
)

GO_TEST_SRCS(manager_test.go)

END()

RECURSE(
    gotest
    mocks
)
