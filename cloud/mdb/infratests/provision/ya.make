PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    certificator.py
    dataproc.py
    dns.py
    iam.py
    kubernetes.py
    provision.py
    values.py
)

PEERDIR(
    contrib/python/boto3
    contrib/python/botocore
    contrib/python/kubernetes
    contrib/python/pyOpenSSL
    contrib/python/retrying
    cloud/bitbucket/private-api/yandex/cloud/priv/dns/v1
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/compute/iam/jwt
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/lockbox
    cloud/mdb/internal/python/logs
    cloud/mdb/internal/python/vault
)

END()
