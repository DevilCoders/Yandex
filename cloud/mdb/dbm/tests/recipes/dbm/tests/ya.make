PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbm/tests/recipes/dbm/recipe.inc)

PEERDIR(
    contrib/python/requests
)

TEST_SRCS(test_recipe.py)

SIZE(MEDIUM)

END()
