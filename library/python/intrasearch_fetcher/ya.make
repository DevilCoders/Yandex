PY23_LIBRARY()

OWNER(
    uruz
    v-sopov
    zivot
)

VERSION(0.2.2)

PEERDIR(
    library/python/ids
)

PY_SRCS(
    TOP_LEVEL
    intrasearch_fetcher/__init__.py
    intrasearch_fetcher/exceptions.py
    intrasearch_fetcher/fetcher.py
)

END()
