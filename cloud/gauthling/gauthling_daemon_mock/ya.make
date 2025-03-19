PY2_PROGRAM()

OWNER(g:cloud)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1
    cloud/gauthling/gauthling_daemon/proto
    cloud/gauthling/gauthling_daemon_mock/lib
)

END()

RECURSE(
    test
)
