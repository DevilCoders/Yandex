OWNER(g:mdb)

PY3_PROGRAM(behave)

STYLE_PYTHON()

PY_SRCS(
    behave.py
    environment.py
)

PY_MAIN(cloud.mdb.pgsync.tests.behave:main)

PEERDIR(
    contrib/python/behave
    contrib/python/docker
    contrib/python/PyYAML
    contrib/python/retrying
    contrib/python/kazoo
    contrib/python/psycopg2
    cloud/mdb/pgsync/tests/steps
)

END()

RECURSE_FOR_TESTS(
    featureset
)
