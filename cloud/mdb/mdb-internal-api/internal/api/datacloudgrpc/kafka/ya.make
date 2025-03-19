GO_LIBRARY()

OWNER(
    g:datacloud
    g:mdb
)

SRCS(
    clusterservice.go
    innertopicservice.go
    models.go
    operationservice.go
    topicservice.go
    versionservice.go
)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(
    console
    gotest
)
