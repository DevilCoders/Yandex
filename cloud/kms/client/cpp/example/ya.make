OWNER(g:cloud-kms)

PROGRAM(kms-example)

PEERDIR(
    cloud/kms/client/cpp
    library/cpp/getopt
    library/cpp/logger/global
)

SRCS(
    main.cpp
)

END()
