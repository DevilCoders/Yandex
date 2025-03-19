GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clusterservice.go
    connectorservice.go
    consoleclusterservice.go
    innertopicservice.go
    models.go
    operationservice.go
    resourcepresetservice.go
    topicservice.go
    userservice.go
)

GO_TEST_SRCS(
    clusterservice_test.go
    models_test.go
)

END()

RECURSE(gotest)
