GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clusterservice.go
    cudservice.go
    models.go
    networkservice.go
)

GO_XTEST_SRCS(cudservice_test.go)

END()

RECURSE(gotest)
