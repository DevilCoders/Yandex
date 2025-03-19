OWNER(g:cloud-iam)

PROGRAM(test-helper)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    contrib/libs/jwt-cpp
    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()
