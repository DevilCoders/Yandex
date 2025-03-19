PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    # Flask and its friends
    contrib/python/Flask
    contrib/python/Flask-Bootstrap
    # marshmallow - 2.19.5
    contrib/python/marshmallow/py2
    contrib/python/webargs/py2
    # PostgreSQL
    contrib/python/psycopg2
    # Misc utils
    contrib/python/requests
    contrib/python/humanfriendly
    contrib/python/pyaml
    contrib/python/retrying
    # Tracing
    contrib/python/opentracing
    contrib/python/opentracing-instrumentation
    contrib/python/Flask-OpenTracing
    contrib/python/jaeger-client
    # Sentry
    contrib/python/sentry-sdk
    contrib/python/sentry-sdk/sentry_sdk/integrations/flask
    # Metrics
    contrib/python/prometheus-client
    # Auth
    library/python/blackbox
    library/python/tvm2
    # python-blackbox require it in its singnature
    library/python/deprecated/ticket_parser2
    # Ya tools
    library/python/svn_version
    # Retries and tracing helpers
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/query_conf
)

ALL_PY_SRCS()

INCLUDE(query_conf.inc)

# Include templates
# https://clubs.at.yandex-team.ru/arcadia/19475
RESOURCE_FILES(
    PREFIX cloud/mdb/dbm/internal/
    templates/base.html
    templates/containers.html
    templates/transfers.html
    templates/volume_backups.html
    templates/volumes.html
)

# Provide default config
RESOURCE(
    cloud/mdb/dbm/app.yaml config/default/app.yaml
)

END()
