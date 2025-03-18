PROGRAM()

OWNER(
    g:antirobot
)

PEERDIR(
    antirobot/daemon_lib
    antirobot/lib
    library/cpp/charset
    library/cpp/getopt
    library/cpp/http/io
    library/cpp/uri
)

SRCS(
    main.cpp
    req2info.cpp
)

END()
