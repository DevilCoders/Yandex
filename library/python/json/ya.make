PY23_LIBRARY()

OWNER(pg)

PEERDIR(
    contrib/python/simplejson
    library/cpp/json/fast_sax
)

PY_SRCS(
    __init__.py
    loads.pyx
)

SRCS(
    loads.cpp
)

END()
