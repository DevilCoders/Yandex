PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/deepdiff
    contrib/python/humanfriendly
    contrib/python/Jinja2
    contrib/python/PyHamcrest
    contrib/python/PyYAML
    contrib/python/semver
    contrib/python/sh
    cloud/mdb/internal/python/compute/disks
    cloud/mdb/internal/python/compute/iam/jwt
    cloud/mdb/internal/python/compute/instances
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/logs
    cloud/mdb/internal/python/vault
)

PY_SRCS(
    compute.py
    context.py
    dataproc.py
    iam.py
    py_api.py
    s3.py
    types.py
    utils.py
)

END()
