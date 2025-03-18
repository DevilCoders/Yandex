PROGRAM()

SRCS(
    testlangmodel.cpp
)

PEERDIR(
    dict/disamb/query_disamb
    kernel/lemmer
    kernel/qtree/request
    kernel/qtree/richrequest
    kernel/reqerror
    library/cpp/charset
    library/cpp/getopt
)

END()
