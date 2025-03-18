PROGRAM()

OWNER(g:antirobot)

PEERDIR(
    contrib/libs/libpcap
    library/cpp/getopt
    library/cpp/http/io
    library/cpp/regex/pcre
)

SRCS(
    main.cpp
)

END()
