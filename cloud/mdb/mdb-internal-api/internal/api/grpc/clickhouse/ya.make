GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backupservice.go
    clusterservice.go
    configmodels.go
    consoleclusterservice.go
    databaseservice.go
    formatschemaservice.go
    mlmodelservice.go
    models.go
    operationservice.go
    resourcepresetservice.go
    userservice.go
    versionsservice.go
)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(gotest)
