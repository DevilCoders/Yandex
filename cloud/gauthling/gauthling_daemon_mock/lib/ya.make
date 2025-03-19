PY2_LIBRARY(gauthling_daemon_mock)

OWNER(g:cloud)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1
    cloud/gauthling/gauthling_daemon/proto
    contrib/python/Flask
)

PY_SRCS(
    NAMESPACE gauthling_daemon_mock
    __init__.py

)

END()
