OWNER(g:mdb-dataproc)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/boto3
    contrib/python/botocore
    contrib/python/cryptography
    contrib/python/PyJWT
    contrib/python/requests
    contrib/python/Jinja2
    contrib/python/pyodbc
    contrib/python/PyYAML
    contrib/python/sshtunnel
    contrib/python/redis
    contrib/python/retrying
    contrib/python/dnspython
    contrib/python/psycopg2
    contrib/python/semver
    contrib/python/hive-metastore-client
    library/python/vault_client
    cloud/bitbucket/private-api/yandex/cloud/priv/metastore/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/vpc/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/microcosm/instancegroup/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1
    cloud/mdb/internal/python/compute/iam/jwt
)

PY_SRCS(
    NAMESPACE tests.helpers
    __init__.py
    base_cluster.py
    compute.py
    compute_driver.py
    crypto.py
    database.py
    deploy_api.py
    dns.py
    generic_intapi.py
    go_internal_api.py
    greenplum_cluster.py
    greenplum.py
    grpcutil/exceptions.py
    grpcutil/service.py
    hadoop_cluster.py
    iam.py
    internal_api.py
    instance_group.py
    job_output.py
    kafka_cluster.py
    matchers.py
    metadb.py
    metastore_cluster.py
    metastore_payload.py
    vault.py
    pillar.py
    utils.py
    s3.py
    ssh_pki.py
    step_helpers.py
    sqlserver.py
    sqlserver_cluster.py
    workarounds.py
)

NO_CHECK_IMPORTS(
    behave.*
)

END()
