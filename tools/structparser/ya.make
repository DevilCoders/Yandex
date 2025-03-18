PROGRAM()

PEERDIR(
    library/cpp/deprecated/fgood
    library/cpp/getopt/small
)

SRCS(
    structinfo.h
    postparser.cpp
    postlexer.cpp
    parser.rl6
    partinfo.h
    lexer.rl6
    parsemain.cpp
    parsestruct.h
)

END()
