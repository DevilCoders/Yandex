PY23_LIBRARY()

OWNER(pg)

PEERDIR(
    skynet/api/copier
)

PY_SRCS(
    NAMESPACE
    api.cqueue
    __init__.py
    __cqudp_runner__.py
    __cqudp_entry__.py
    log.py
)

END()
