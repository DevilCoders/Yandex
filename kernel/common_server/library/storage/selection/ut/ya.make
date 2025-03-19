UNITTEST_FOR(kernel/common_server/library/storage/selection)

OWNER(g:cs_dev)

SIZE(SMALL)

PEERDIR(
    kernel/common_server/migrations
    kernel/common_server/library/storage/postgres
)

SRCS(
    reader_ut.cpp
)

DATA(
    arcadia/kernel/common_server/library/storage/selection/ut/migrations
    arcadia/kernel/common_server/migrations/common_migrations
    arcadia/library/recipes/postgresql
)

SET(PG_MIGRATIONS_DIR kernel/common_server/library/storage/selection/ut/migrations)
SET(PG_SCHEMA_MIGRATIONS_DIR kernel/common_server/migrations/common_migrations)

INCLUDE(${ARCADIA_ROOT}/library/recipes/postgresql/recipe.inc)

END()
