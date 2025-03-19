GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    base_action.go
    errors.go
    license_check.go
    types.go
)

GO_TEST_SRCS(
    license_check_test.go
)

END()

RECURSE(
    gotest
)
