PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(2.3)

PEERDIR(
    contrib/python/requests
    contrib/python/six
    contrib/python/setuptools
)

PY_SRCS(
    TOP_LEVEL
    yandex_tracker_client/__init__.py
    yandex_tracker_client/client.py
    yandex_tracker_client/collections.py
    yandex_tracker_client/connection.py
    yandex_tracker_client/exceptions.py
    yandex_tracker_client/objects.py
    yandex_tracker_client/settings.py
    yandex_tracker_client/uriutils.py
)

END()
