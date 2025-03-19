OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/jsonschema
    contrib/python/tenacity
    contrib/python/opentracing
    contrib/python/jaeger-client
    contrib/python/opentracing-instrumentation
    contrib/python/grpcio-opentracing
)

PY_SRCS(
    TOP_LEVEL
    dbaas_common/__init__.py
    dbaas_common/config.py
    dbaas_common/dict.py
    dbaas_common/retry.py
    dbaas_common/tracing.py
    dbaas_common/tskv.py
    dbaas_common/worker.py
)

END()

RECURSE_FOR_TESTS(
    test
)
