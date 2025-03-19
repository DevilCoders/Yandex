GO_LIBRARY()

OWNER(g:cloud-kms)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1/discovery
    cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1
)

SRCS(
    atomictime.go
    balancer.go
    client.go
    credentials.go
    discovery.go
    fixedtimewindow.go
    logging.go
    options.go
    resolver.go
    retry.go
    validation.go
)

GO_TEST_SRCS(
    client_test.go
    fixedtimewindow_test.go
)

END()

RECURSE(
    cmd
    gotest
    kmsmock
)
