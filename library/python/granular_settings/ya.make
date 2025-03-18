PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(1.4)

PEERDIR(
    contrib/python/six
    library/python/yenv
    library/python/resource
)

PY_SRCS(
    TOP_LEVEL
    granular_settings/__init__.py
    granular_settings/settings.py
    granular_settings/utils.py
)

END()
