PY3_PROGRAM(ssh)

OWNER(g:rtc-sysdev)

PY_SRCS(
    MAIN
    ssh.py
)

PEERDIR(
    library/python/ssh_client
)

END()
