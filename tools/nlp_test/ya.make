PROGRAM()

OWNER(leo)

SRCS(
    nlp_test.cpp
)

PEERDIR(
    kernel/keyinv/invkeypos
    library/cpp/charset
    library/cpp/getopt
    library/cpp/html/pdoc
    library/cpp/numerator
    library/cpp/token
)

END()

RECURSE(
    tests
)
