UNITTEST()

OWNER(g:fintech-bnpl)

PEERDIR(
    kernel/common_server/ut
    kernel/common_server/ut/scripts
    kernel/common_server/solutions/fallback_proxy/src/server
    kernel/common_server/solutions/fallback_proxy/ut/commands
    library/cpp/testing/mock_server
    kernel/common_server/library/storage/local
    kernel/common_server/library/simpledate
)

IF (NOT OS_WINDOWS)
    PEERDIR(
        kernel/common_server/library/storage/postgres
    )
ENDIF()

SRCS(
    simple_ut.cpp
    common.cpp
)

DATA(
    arcadia/kernel/common_server/migrations/common_migrations
    arcadia/kernel/common_server/ut/base_migrations
    arcadia/kernel/common_server/solutions/fallback_proxy/migrations
)


REQUIREMENTS(network:full)

TAG(ya:sandbox_coverage)

SET(PG_MIGRATIONS_DIR kernel/common_server/ut/base_migrations)

SET(PG_SCHEMA_MIGRATIONS_DIR kernel/common_server/migrations/common_migrations,kernel/common_server/solutions/fallback_proxy/migrations)

INCLUDE(${ARCADIA_ROOT}/library/recipes/postgresql/recipe.inc)

TIMEOUT(600)

SIZE(MEDIUM)

FORK_SUBTESTS()

##SPLIT_FACTOR(20)

END()
