LIBRARY()

OWNER(
    yuryalexkeev
    g:clustermaster
)

PEERDIR(
    library/cpp/archive
    library/cpp/coroutine/engine
    library/cpp/http/server
    library/cpp/terminate_handler
    library/cpp/http/misc
    library/cpp/string_utils/quote
    library/cpp/deprecated/atomic
)

SRCS(
    daemon.cpp
    dirut.cpp
    file_reopener_by_signal.cpp
    pidfile.cpp
    http_util.cpp
    static_reader.cpp
)

END()
