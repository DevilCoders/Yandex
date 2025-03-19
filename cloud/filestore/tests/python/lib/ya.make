PY23_LIBRARY()

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/filestore/config

    cloud/filestore/public/sdk/python/client
    cloud/filestore/public/sdk/python/protos

    contrib/python/requests
    contrib/python/retrying

    kikimr/ci/libraries
)

PY_SRCS(
    client.py
    common.py
    endpoint.py
    http_proxy.py
    loadtest.py
    nfs_ganesha.py
    server.py
    server_config.py
    vhost.py
)

END()
