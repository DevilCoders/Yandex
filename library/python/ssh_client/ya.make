PY23_LIBRARY()

OWNER(g:rtc-sysdev)

PEERDIR(
    contrib/python/paramiko
)

PY_SRCS(__init__.py)

END()

RECURSE(
    ssh
)

RECURSE_FOR_TESTS(tests)
