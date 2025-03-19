PY2TEST()
OWNER(g:mdb)

DEPENDS(
    cloud/mdb/salt-tests/obeyjinja
)

DATA(
    arcadia/cloud/mdb/salt-tests/obeyjinja/.obeyjinjarc
    arcadia/cloud/mdb/salt/salt/components
)

TEST_SRCS(
    lint_sls.py
)

TIMEOUT(60)
SIZE(SMALL)
END()

RECURSE_FOR_TESTS(
    pushclient
    pushclient2
    mongodb
    redis
    vector
)
