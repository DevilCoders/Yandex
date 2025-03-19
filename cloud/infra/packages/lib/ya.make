OWNER(g:cloud-infra)

PY23_LIBRARY()

PEERDIR(
    contrib/python/urllib3
)

PY_SRCS(
    TOP_LEVEL
    ycinfra.py
)

END()

RECURSE(tests)
