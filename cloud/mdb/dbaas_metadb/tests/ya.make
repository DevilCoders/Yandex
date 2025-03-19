PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

DEPENDS(
    contrib/python/yandex-pgmigrate/bin
)

DEPENDS(
    cloud/mdb/dbaas_metadb/bin
)

PEERDIR(
    library/python/testing/behave
    contrib/python/PyYAML
    contrib/python/PyHamcrest
    contrib/python/psycopg2
    contrib/python/colorlog
    contrib/python/Jinja2
    cloud/mdb/dbaas_metadb/tests/helpers
)

SIZE(MEDIUM)

NO_CHECK_IMPORTS()

END()

RECURSE(
    grants
)
