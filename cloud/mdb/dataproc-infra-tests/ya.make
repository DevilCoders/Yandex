OWNER(g:mdb-dataproc)

PY3_PROGRAM(infratests)

STYLE_PYTHON()

PY_SRCS(
    MAIN
    run.py
)

PEERDIR(
    contrib/python/behave
    contrib/python/confluent-kafka
    cloud/mdb/dataproc-infra-tests/tests
)

END()

RECURSE(
    tests
)
