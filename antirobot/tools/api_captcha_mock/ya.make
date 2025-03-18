PROGRAM()

OWNER(
    g:antirobot
)

PEERDIR(
    library/cpp/getopt
    library/cpp/http/server
    library/cpp/http/misc
    library/cpp/cgiparam
    library/cpp/json
)

SRCS(
    main.cpp
    options.cpp
    path_handlers.cpp
    image.cpp
)

END()
