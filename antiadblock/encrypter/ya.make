LIBRARY()

OWNER(g:antiadblock)

PEERDIR(
    yweb/webdaemons/icookiedaemon/icookie_lib
)

SRCS(
    encryption.cpp
)

PY_SRCS(
    __init__.py
    encryption.pyx
)

END()

RECURSE_FOR_TESTS (
    tests
)
