GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backupservice.go
    clusterservice.go
    consoleclusterservice.go
    models.go
    operationservice.go
    resourcepresetservice.go
)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(gotest)
