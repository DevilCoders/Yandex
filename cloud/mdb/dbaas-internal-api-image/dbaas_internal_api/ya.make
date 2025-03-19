OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    # A
    library/python/resource
    library/python/svn_version
    # Cloud
    # yc-as-client
    cloud/iam/accessservice/client/python
    # yc-requests
    cloud/gauthling/yc_requests/lib
    cloud/gauthling/yc_auth/lib
    cloud/mdb/internal/python/compute/host_groups
    cloud/mdb/internal/python/compute/host_type
    cloud/mdb/internal/python/compute/instances
    cloud/mdb/internal/python/compute/images
    # VPC
    cloud/mdb/internal/python/compute/vpc
    # Flask and it's friends
    contrib/python/Flask
    contrib/python/Werkzeug
    contrib/python/Flask-RESTful
    contrib/python/flask-appconfig
    contrib/python/prometheus-flask-exporter
    contrib/python/webargs/py2
    contrib/python/apispec/py2
    # marshmallow - 2.19.5
    contrib/python/marshmallow/py2
    # Retry utils
    cloud/mdb/dbaas_python_common
    contrib/python/tenacity
    # Databases
    contrib/python/psycopg2
    contrib/python/clickhouse-cityhash
    contrib/python/clickhouse-driver
    # S3
    contrib/python/boto3
    # Cryptography
    contrib/python/cryptography
    contrib/python/PyNaCl
    contrib/python/argon2-cffi
    # Time utilities
    contrib/python/aniso8601
    contrib/python/dateutil
    contrib/python/pytz
    # Misc utilities
    cloud/mdb/internal/python/logs
    contrib/python/attrs
    contrib/python/requests
    contrib/python/humanfriendly
    contrib/python/lark-parser
    contrib/python/lz4
    contrib/python/sshpubkeys
    contrib/python/typing-extensions
    # Sentry
    contrib/python/raven
    # OpenTracing
    contrib/python/opentracing
    contrib/python/Flask-OpenTracing
    contrib/python/service-identity
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/apis
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/core
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/dbs
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/health
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/utils
)

PY_SRCS(
    NAMESPACE dbaas_internal_api
    __init__.py
    converters.py
    default_config.py
)

END()

RECURSE(
    apis
    core
    dbs
    health
    modules
    utils
)
