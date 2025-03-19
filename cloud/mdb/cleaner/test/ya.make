PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

FORK_SUBTESTS()

REQUIREMENTS(cpu:4)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/internal/dbteststeps/deps.inc)

SIZE(MEDIUM)

PEERDIR(
    cloud/mdb/cleaner/internal
    cloud/mdb/recipes/postgresql/lib
    contrib/python/pytest-mock
    library/python/resource
)

TEST_SRCS(
    test_config.py
    test_metadb.py
    test_internal_api.py
)

RESOURCE(
    example_config.yaml config
)

END()
