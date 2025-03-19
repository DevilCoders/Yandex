OWNER(g:cloud-kms)

PROGRAM(benchcrypto)

PEERDIR(
    contrib/libs/openssl
    library/cpp/getopt
    library/cpp/logger/global
    ydb/core/blobstorage/crypto
)

SRCS(
    main.cpp
)

END()
