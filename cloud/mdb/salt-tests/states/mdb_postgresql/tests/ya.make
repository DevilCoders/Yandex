PY23_TEST()
OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/salt-tests/states/mdb_postgresql/recipe/recipe.inc)

PEERDIR(
    cloud/mdb/internal/python/pytest
    contrib/python/psycopg2
    cloud/mdb/salt/salt/_states
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
    contrib/python/mock
)

TEST_SRCS(
   test_drop_user.py
)

SIZE(MEDIUM)

END()
