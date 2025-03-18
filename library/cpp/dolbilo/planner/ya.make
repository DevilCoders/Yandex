LIBRARY()

OWNER(
    g:base
    darkk
    mvel
)

PEERDIR(
    library/cpp/containers/intrusive_avl_tree
    library/cpp/coroutine/engine
    library/cpp/digest/old_crc
    library/cpp/dolbilo
    library/cpp/eventlog
    library/cpp/getopt/small
    library/cpp/http/fetch
    library/cpp/http/io
    library/cpp/streams/factory
    library/cpp/string_utils/base64
    search/idl
)

SRCS(
    plain.cpp
    loadlog.cpp
    accesslog.cpp
    eventlog.cpp
    planlog.cpp
    pcapdump.cpp
    fullreqs.cpp
    phantom.cpp
)

END()
