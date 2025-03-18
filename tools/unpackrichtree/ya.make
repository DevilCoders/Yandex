PROGRAM()

OWNER(g:wizard)

SRCS(unpackrichtree.cpp)

PEERDIR(
    library/cpp/charset
    library/cpp/getopt
    kernel/qtree/richrequest
    library/cpp/string_utils/quote
)

END()
