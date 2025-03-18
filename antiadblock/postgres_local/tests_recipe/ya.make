PY2TEST()

OWNER(g:antiadblock solovyev)

INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/recipe.inc)

DEPENDS(
  antiadblock/postgres_local/recipe
)

TEST_SRCS(
   test_postgres_recipe.py
)

PEERDIR(
    contrib/python/psycopg2
    contrib/python/sqlalchemy/sqlalchemy-1.2
)

REQUIREMENTS(network:full)

SIZE(MEDIUM)

END()
