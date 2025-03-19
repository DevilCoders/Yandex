GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    auth.go
    auth_interceptor.go
)

GO_TEST_SRCS(
    auth_interceptor_test.go
    auth_test.go
)

END()

RECURSE(gotest)
