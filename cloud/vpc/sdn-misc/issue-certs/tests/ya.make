PY3TEST()

SIZE(SMALL)

OWNER(
    sklyaus
)

PEERDIR(
    contrib/python/cryptography
    contrib/python/pem

    contrib/python/responses

    cloud/vpc/sdn-misc/issue-certs/lib
)

TEST_SRCS(
    conftest.py

    test_secret_service.py
    test_yc_crt.py
)

NO_CHECK_IMPORTS(
    pem.twisted
)

END()
