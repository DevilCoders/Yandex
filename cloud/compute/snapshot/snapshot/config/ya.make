GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.go
    endpoint.go
)

GO_TEST_SRCS(config_test.go)

END()

RECURSE(gotest)
