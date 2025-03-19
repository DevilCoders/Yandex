PY23_LIBRARY()

OWNER(g:facts)

PY_SRCS(
    __init__.py
    wrapper.pyx
)

PEERDIR(
    contrib/python/six
    library/cpp/scheme
    kernel/facts/serp_parser
)

END()
