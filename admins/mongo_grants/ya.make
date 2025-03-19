OWNER(g:music-sre)

PROGRAM(mongo-grants)

PY_SRCS(
    TOP_LEVEL
    MAIN main.py
    core.py
    grants.py
    helpers.py
)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/pymongo
)

END()
