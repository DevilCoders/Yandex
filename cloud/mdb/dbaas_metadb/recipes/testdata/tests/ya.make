PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/testdata/recipe.inc)

PEERDIR(
    contrib/python/psycopg2
)

TEST_SRCS(test_recipe.py)

DATA(arcadia/cloud/mdb/salt/pillar/metadb_default_versions.sls)

DATA(arcadia/cloud/mdb/salt/pillar/metadb_default_alert.sls)

REQUIREMENTS(network:full)

SIZE(MEDIUM)

END()
