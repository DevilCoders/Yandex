OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.utils.cluster
    __init__.py
    create.py
    delete.py
    get.py
    move.py
    update.py
)

END()
