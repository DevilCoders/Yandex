PY3TEST()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/billing/db/recipe/recipe.inc)
DATA(arcadia/cloud/mdb/billing/db)

PEERDIR(contrib/python/psycopg2)

TEST_SRCS(
   test_recipe.py
)

SIZE(MEDIUM)

END()
