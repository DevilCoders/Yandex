PY3_PROGRAM()

OWNER(g:cloud_analytics)

PEERDIR(
    contrib/python/arrow
    contrib/python/click
    contrib/python/requests
    contrib/python/tqdm
)

PY_SRCS(MAIN __init__.py)

END()
