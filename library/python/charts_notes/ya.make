PY23_LIBRARY()

OWNER(g:maps-jams voidex aydarboss)

PEERDIR(
    contrib/python/dateutil
    contrib/python/requests
    contrib/python/retry
)

PY_SRCS(
    __init__.py
    actions.py
    colors.py
    comment.py
    notes.py
    utils.py
)

END()
