OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/manager/v1
)

PY_SRCS(
    NAMESPACE dbaas_internal_api.utils.dataproc_manager
    __init__.py
    api.py
    client.py
    health.py
)

END()
