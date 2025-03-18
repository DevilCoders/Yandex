PY23_LIBRARY()

OWNER(
    g:statlibs
    g:statinfra
)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/requests
    contrib/python/six
    statbox/libstatbox/python
)

PY_SRCS(
    TOP_LEVEL
    statface_client/__init__.py
    statface_client/api.py
    statface_client/base_client.py
    statface_client/constants.py
    statface_client/dictionary.py
    statface_client/errors.py
    statface_client/interface.py
    statface_client/report/__init__.py
    statface_client/report/api.py
    statface_client/report/config.py
    statface_client/tools.py
    statface_client/utils.py
    statface_client/version.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
