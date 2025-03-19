PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

FORK_TESTS()

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/grpcutil
    contrib/python/PyJWT
)

PY_SRCS(
    __init__.py
    iam_jwt.py
)

END()
