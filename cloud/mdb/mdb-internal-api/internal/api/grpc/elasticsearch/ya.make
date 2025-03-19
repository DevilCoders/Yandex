GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    authservice.go
    backupservice.go
    clusterservice.go
    consoleclusterservice.go
    extensionservice.go
    models.go
    operationservice.go
    resourcepresetservice.go
    userservice.go
)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(gotest)
