OWNER(g:mdb)

PY3TEST()

STYLE_PYTHON()

PEERDIR(
    library/python/testing/yatest_common
    contrib/python/PyHamcrest
    contrib/python/Jinja2
    contrib/python/jsonpath-rw
    contrib/python/pytest-mock
    cloud/mdb/solomon-charts/internal/lib
    cloud/mdb/solomon-charts/internal/lib/filters
)

TEST_SRCS(
    __init__.py
    filtration.py
    health.py
    template_loader.py
    loading_jinja_filters.py
)

DATA(
    arcadia/cloud/mdb/solomon-charts/templates
    arcadia/cloud/mdb/solomon-charts/solomon.json
)

TIMEOUT(60)

SIZE(SMALL)

END()
