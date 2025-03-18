PROGRAM()

OWNER(pg)

PEERDIR(
    library/cpp/blockcodecs
    library/cpp/cgiparam
    library/cpp/coroutine/engine
    library/cpp/coroutine/listener
    library/cpp/digest/md5
    library/cpp/http/misc
    library/cpp/http/server
    search/cache
)

SRCS(
    main.cpp
    httpsrv.cpp
)

END()
