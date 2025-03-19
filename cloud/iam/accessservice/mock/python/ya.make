PY2_PROGRAM(accessservice-mock)

OWNER(g:cloud-iam)

PY_SRCS(
    NAMESPACE yc_as_mock
    __main__.py
    mock_service.py
    control_service.py
)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1

    contrib/python/Flask
)

END()
