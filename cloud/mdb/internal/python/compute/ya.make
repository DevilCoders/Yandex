PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/operation
    cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1
    contrib/libs/grpc/src/python/grpcio_status
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/dbaas_python_common
)

PY_SRCS(
    __init__.py
    models.py
    pagination.py
    service.py
)

END()

RECURSE(
    clouds
    disk_placement_groups
    disks
    dns
    folders
    host_groups
    host_type
    iam
    images
    instances
    operations
    placement_groups
    vpc
)
