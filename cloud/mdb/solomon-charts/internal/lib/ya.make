PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/Jinja2
    contrib/python/jsonpath-rw
    contrib/python/requests
    cloud/mdb/solomon-charts/internal/lib/filters
)

PY_SRCS(
    __init__.py
    render.py
    solomon_loader.py
    config.py
    upload.py
)

END()

RECURSE(
    filters
)
