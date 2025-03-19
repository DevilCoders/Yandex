UNITTEST_FOR(kernel/common_server/library/storage/postgres)

OWNER(g:cs_dev)

SIZE(SMALL)

PEERDIR(
    kernel/common_server/migrations
)

SRCS(
    structured_ut.cpp
)

REQUIREMENTS(
    network:full
)

SET(PG_MIGRATIONS_DIR kernel/common_server/library/storage/postgres/ut/migrations)

INCLUDE(${ARCADIA_ROOT}/library/recipes/postgresql/recipe.inc)

END()
