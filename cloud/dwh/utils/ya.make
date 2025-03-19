OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    contrib/python/colorlog
    contrib/python/pytz
)

PY_SRCS(
    coroutines.py
    datetimeutils.py
    log.py
    misc.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
