PY3_LIBRARY()

PY_SRCS(
    mutator.py
    __init__.py
)


PEERDIR(
    cloud/ai/lib/python/log
    contrib/python/pylev
    contrib/python/pymorphy2
)

END()
