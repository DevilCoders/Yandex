PROGRAM()

OWNER(
    g:antirobot
)

PEERDIR(
    antirobot/daemon_lib
    antirobot/idl
    antirobot/lib
    library/cpp/getopt
    library/cpp/http/misc
    library/cpp/http/io
)

SRCS(
    main.cpp
    ip2backend.cpp
    options.cpp
)

END()
