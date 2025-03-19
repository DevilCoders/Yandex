PROGRAM()

OWNER(g:mssngr)

PEERDIR(
    mssngr/router/lib/common
    mssngr/router/lib/load_test
    mssngr/router/lib/transport
    ml/neocortex/neocortex_lib
    library/cpp/getopt
    library/cpp/http/simple
    library/cpp/json
    library/cpp/json/writer
    library/cpp/deprecated/atomic
)

SRCS(
    auto_teamlead.cpp
    main.cpp
    neocortex_bot.cpp
)

END()
