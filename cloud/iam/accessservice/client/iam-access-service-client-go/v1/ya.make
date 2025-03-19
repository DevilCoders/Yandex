GO_LIBRARY()

OWNER(g:cloud-iam)

PEERDIR(
    cloud/iam/accessservice/client/iam-access-service-api-proto/v1/go
)

SRCS(
    client.go
)

GO_TEST_SRCS(
    client_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
