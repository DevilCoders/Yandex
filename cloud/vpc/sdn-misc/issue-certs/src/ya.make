PY3_PROGRAM(yc-issue-certs)

OWNER(
    sklyaus
)

PEERDIR(
    cloud/vpc/sdn-misc/issue-certs/lib
)

NO_CHECK_IMPORTS(
    pem.twisted
)

PY_SRCS(
    __main__.py
)

END()
