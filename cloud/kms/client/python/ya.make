PY23_LIBRARY(kmsclient)

OWNER(g:cloud-kms)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1
)

PY_SRCS(
    NAMESPACE kmsclient
    __init__.py
    client.py
)

END()

RECURSE(example)
