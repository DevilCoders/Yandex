OWNER(g:mdb-dataproc)

PY3_PROGRAM(infratests)

STYLE_PYTHON()

PY_SRCS(
    MAIN
    run.py
    config.py
    environment.py
)

PEERDIR(
    contrib/python/behave
    cloud/mdb/infratests/test_helpers
    cloud/mdb/infratests/provision
)

END()

RECURSE(
    provision
    test_helpers
)
