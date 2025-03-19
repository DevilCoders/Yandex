OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE utils
    __init__.py
    bgsave.py
    command.py
    config.py
    connection.py
    ctl_logging.py
    file_ops.py
    exception.py
    process.py
    persistence.py
    redis_server.py
    state.py
    stop.py
    timing.py
)

PEERDIR(
    contrib/python/redis
)

END()
