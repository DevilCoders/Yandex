PROGRAM()

OWNER(
    g:antirobot
)

PEERDIR(
    infra/yp_service_discovery/libs/sdlib/server_mock
    library/cpp/cgiparam
    library/cpp/getopt
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/json
    library/cpp/json/writer
    library/cpp/string_utils/base64
)

SRCS(
    main.cpp
)

END()
