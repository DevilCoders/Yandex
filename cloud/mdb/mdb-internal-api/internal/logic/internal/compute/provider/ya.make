GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    host_group.go
    host_type.go
    network.go
    provider.go
)

GO_TEST_SRCS(
    host_group_test.go
    network_test.go
)

END()

RECURSE(gotest)
