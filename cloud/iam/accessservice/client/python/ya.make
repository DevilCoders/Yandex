OWNER(g:cloud-iam)

PY23_LIBRARY(yc_as_python_client)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1
)

PY_SRCS(
    NAMESPACE yc_as_client
    __init__.py
    client.py
    entities.py
    exceptions.py
)

END()

RECURSE_FOR_TESTS(
    ut
)
