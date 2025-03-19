GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.go
    repository.go
)

GO_TEST_SRCS(
    config_test.go
    mon_test.go
)

END()

RECURSE(gotest)
