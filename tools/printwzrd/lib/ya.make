OWNER(g:wizard)

LIBRARY()

PEERDIR(
    kernel/reqerror
    library/cpp/charset
    library/cpp/getopt/small
    library/cpp/streams/factory
    library/cpp/svnversion
    search/idl
    search/wizard
    search/wizard/core
    search/wizard/face
    ysite/yandex/reqdata
    kernel/qtree/request
    kernel/qtree/richrequest
    library/cpp/cgiparam
    library/cpp/string_utils/quote
)

SRCS(
    defaultcfg.cpp
    options.cpp
    prepare_request.cpp
    printer.cpp
    verify_gazetteer.cpp
)

END()
