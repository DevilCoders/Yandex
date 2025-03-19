OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/argon2-cffi
    contrib/python/tenacity
    contrib/python/jsonschema
    contrib/python/requests
    cloud/mdb/dbaas_python_common
    cloud/mdb/dbaas_metadb/tests/helpers
    cloud/mdb/recipes/postgresql/lib
)

PY_SRCS(
    NAMESPACE util
    __init__.py
    config.py
    fs_cleanup.py
    logs.py
    metadb.py
    mk_api_dirs.py
    project.py
    runner.py
)

END()
