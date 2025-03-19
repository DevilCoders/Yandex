GO_LIBRARY()

OWNER(g:cloud-kms)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1/keystore
    cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1
)

SRCS(
    client.go
)

END()
