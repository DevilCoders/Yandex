PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(2.5)

PEERDIR(
    contrib/python/requests
    contrib/python/six
    contrib/python/setuptools

    library/python/yandex_tracker_client
)

PY_SRCS(
    TOP_LEVEL
    startrek_client/__init__.py
    startrek_client/client.py
    startrek_client/collections.py
    startrek_client/connection.py
    startrek_client/exceptions.py
    startrek_client/objects.py
    startrek_client/settings.py
)

END()
