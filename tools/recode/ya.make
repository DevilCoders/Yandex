PROGRAM()

OWNER(tejblum)

PEERDIR(
    library/cpp/charset
    library/cpp/getopt/small
    library/cpp/xml/encode
)

SRCS(
    recode.cpp
)

END()

RECURSE(tests)
