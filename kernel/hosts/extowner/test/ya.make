PROGRAM()

OWNER(
    karina-usm
    g:antiinfra
)

PEERDIR(
    kernel/hosts/extowner
    library/cpp/getopt/small
)

SRCDIR(kernel/hosts/extowner/test/python)

SRCS(
    extowner_perf_test.cpp
)

END()
