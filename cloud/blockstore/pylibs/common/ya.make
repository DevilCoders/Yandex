PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    helpers.py
    logger.py
    profiler.py
    retry.py
    ssh.py
)

PEERDIR(
    contrib/python/paramiko
    contrib/python/urllib3
)

END()
