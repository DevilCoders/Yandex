PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    sandbox.py
)

PEERDIR(
    sandbox/common/proxy
    sandbox/common/rest
)

END()
