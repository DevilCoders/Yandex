GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    state.go
    updater.go
)

GO_TEST_SRCS(updater_test.go)

END()

RECURSE(
    gotest
    mocks
)
