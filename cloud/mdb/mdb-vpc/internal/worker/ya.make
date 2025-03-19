GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    create_network.go
    create_network_connection.go
    delete_network.go
    delete_network_connection.go
    import_vpc.go
    iteration.go
    operation.go
    worker.go
)

GO_XTEST_SRCS(iteration_test.go)

END()

RECURSE(
    config
    gotest
    network
)
