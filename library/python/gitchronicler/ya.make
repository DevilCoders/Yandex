PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(0.14)

PEERDIR(
    contrib/python/dateutil
)

PY_SRCS(
    TOP_LEVEL
    gitchronicler/__init__.py
    gitchronicler/ctl.py
    gitchronicler/formatter.py
)

END()
