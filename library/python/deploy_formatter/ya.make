PY23_LIBRARY()

OWNER(
    g:deploy
    g:deploy-orchestration
)

PY_SRCS(
    __init__.py
)

PEERDIR(
    contrib/python/six
    contrib/python/ujson
)

END()
