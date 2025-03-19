OWNER(
    g:base
    g:wizard
    leo
    onpopov
)

LIBRARY()

NO_WSHADOW()

SRCS(
    analyzer.cpp
    fixreq.cpp
    nodebase.cpp
    printreq.cpp
    req_node.cpp
    req_pars.y
    reqattrlist.cpp
    reqlenlimits.cpp
    reqscan.h
    reqscan.cpp
    request.cpp
    simpleparser.h
)

# in case rebuild doesn't work, generate ragel file manually (run ya make -v to see ragel generation command)
# # # ENABLE(REBUILD_REQSCAN)

IF (REBUILD_REQSCAN)
    SRCS(
        reqscan1.rl6
    )
    SET(RAGEL6_FLAGS -CT1)

ELSE()
    SRCS(
        generated/reqscan1.cpp
    )
ENDIF()

PEERDIR(
    kernel/search_daemon_iface
    kernel/reqerror
    library/cpp/charset
    library/cpp/token
    library/cpp/tokenclassifiers
    library/cpp/tokenizer
    library/cpp/uri
    library/cpp/yconf
    ysite/yandex/common
    library/cpp/string_utils/url
)

GENERATE_ENUM_SERIALIZATION(nodebase.h)

END()
