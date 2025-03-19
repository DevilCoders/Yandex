GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clusterservice.go
    models.go
)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(gotest)
