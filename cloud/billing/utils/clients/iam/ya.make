PY23_LIBRARY()

OWNER(g:cloud-billing)

PY_SRCS(
    yc_token_client.py
)
PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    cloud/billing/utils/clients/grpc
)
END()
