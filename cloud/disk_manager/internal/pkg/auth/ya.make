OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    auth.go
)

GO_TEST_SRCS(
    auth_test.go
)

END()

RECURSE(
    config
)

RECURSE_FOR_TESTS(
    tests
)
