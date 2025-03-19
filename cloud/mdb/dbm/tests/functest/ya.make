PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbm/tests/recipes/dbm/recipe.inc)

DEPENDS(
    contrib/python/yandex-pgmigrate/bin
)

PEERDIR(
    library/python/testing/behave
    library/python/testing/yatest_common
    contrib/python/PyYAML
    contrib/python/PyHamcrest
    contrib/python/psycopg2
    contrib/python/retrying
    contrib/python/requests
    cloud/mdb/dbaas_python_common
)

SIZE(MEDIUM)

NO_CHECK_IMPORTS()

END()
