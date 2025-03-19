GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    errors.go
    localFiler.go
)

GO_TEST_SRCS(localFiler_test.go)

END()

RECURSE(gotest)
