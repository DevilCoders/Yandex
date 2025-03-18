PY2_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    __init__.py
    _BaseModule.py
    AdblocksExtVersion.py
    tools/__init__.py
    tools/github_releases.py
)

PEERDIR(
    contrib/python/requests
)

END()
