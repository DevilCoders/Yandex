PY23_LIBRARY()

OWNER(g:rtc-sysdev)

TEST_SRCS(test.py)

PEERDIR(
    library/python/ssh_client
)

END()

RECURSE_FOR_TESTS(
    py2
    py3
)
