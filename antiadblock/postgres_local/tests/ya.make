PY2TEST()

OWNER(g:antiadblock solovyev)

INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/postgresql_bin.inc)

DATA(
    arcadia/antiadblock/postgres_local/tests/test_migrations
)

TEST_SRCS(
   test_postgres.py
)

PEERDIR(
    antiadblock/postgres_local
)

REQUIREMENTS(network:full)

SIZE(MEDIUM)



END()
