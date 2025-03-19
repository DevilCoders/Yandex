PY3_PROGRAM(kafka-agent)

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    MAIN
    kafka_agent.py
)

PEERDIR(
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/logs
    cloud/mdb/internal/python/compute/iam/jwt
    cloud/mdb/kafka_agent/internal
    contrib/python/Flask
    contrib/python/pyaml
)

END()
