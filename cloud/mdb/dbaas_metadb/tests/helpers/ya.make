OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/behave
    contrib/python/PyYAML
    contrib/python/PyHamcrest
    contrib/python/psycopg2
)

PY_SRCS(
    __init__.py
    cluster_changes.py
    connect.py
    databases.py
    locate.py
    migrations.py
    populate.py
    queries.py
    types.py
)

END()
