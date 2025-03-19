GO_LIBRARY()

OWNER(
    g:datacloud
    g:mdb
)

PEERDIR(cloud/mdb/datacloud/private_api/datacloud/kafka/console/v1)

SRCS(
    cloudservice.go
    clusterservice.go
)

GO_TEST_SRCS(clusterservice_test.go)

END()

RECURSE(gotest)
