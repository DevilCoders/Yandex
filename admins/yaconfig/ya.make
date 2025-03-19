PY23_LIBRARY(yaconfig)

OWNER(g:music-sre)

PY_SRCS(
    __init__.py
)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/dotmap
)

END()
