UNITTEST_FOR(kernel/common_server/library/openssl)

OWNER(g:cs_dev)

SIZE(SMALL)

DATA(arcadia/kernel/common_server/library/openssl/ut/CA.pem)

SRCS(
    oio_ut.cpp
    rsa_ut.cpp
)

END()
