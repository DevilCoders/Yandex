GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(
    models.go
    topology.go
)

GO_TEST_SRCS(
    models_marshal_test.go
    models_test.go
    topology_test.go
)

END()

RECURSE(
    gotest
    health
    role
    service
)
