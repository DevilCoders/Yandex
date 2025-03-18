PROGRAM()

OWNER(axc)

SRCS(langdiscr-test.cpp)

PEERDIR(
    kernel/lemmer
    kernel/lemmer/dictlib
    kernel/qtree/request
    kernel/qtree/richrequest
    kernel/reqerror
    library/cpp/charset
    library/cpp/getopt
    library/cpp/langs
)

END()

RECURSE(tests)
