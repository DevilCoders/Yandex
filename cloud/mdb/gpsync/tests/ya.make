OWNER(g:mdb)

PY3_PROGRAM(behave)

STYLE_PYTHON()

PY_SRCS(
    behave.py
    environment.py
)

PY_MAIN(cloud.mdb.gpsync.tests.behave:main)

PEERDIR(
    contrib/python/behave
    contrib/python/docker
    contrib/python/PyYAML
    contrib/python/retrying
    contrib/python/kazoo
    contrib/python/psycopg2
    contrib/python/parsedatetime
    cloud/mdb/gpsync/tests/steps
)

END()

RECURSE_FOR_TESTS(
    featureset
)

