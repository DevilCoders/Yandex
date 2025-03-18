PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(0.13.0)

PEERDIR(
    contrib/python/requests
    contrib/python/six
    library/python/tvm2
)

PY_SRCS(
    TOP_LEVEL
    django_mds/__init__.py
    django_mds/client.py
    django_mds/fields.py
    django_mds/models.py
    django_mds/storage.py
)

END()
