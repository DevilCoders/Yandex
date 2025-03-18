PROGRAM()

OWNER(akhropov)

PEERDIR(
    library/cpp/getopt
    library/cpp/token/serialization
    ysite/yandex/reqanalysis
)

SRCS(
    test_norm.cpp
)

END()

RECURSE(
    tests
)
