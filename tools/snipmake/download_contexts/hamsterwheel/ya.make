PROGRAM()

OWNER(
    pzuev
    g:snippets
)

PEERDIR(
    kernel/qtree/richrequest
    kernel/snippets/idl
    library/cpp/cgiparam
    library/cpp/charset
    library/cpp/colorizer
    library/cpp/deprecated/split
    library/cpp/getopt
    library/cpp/scheme
    library/cpp/string_utils/base64
    search/idl
    search/session/compression
    tools/snipmake/download_contexts/common
    tools/snipmake/reqrestr
    tools/snipmake/snipdat
    ysite/yandex/common
)

SRCS(
    hamsterwheel.cpp
    jobqueue.cpp
    outputqueue.cpp
    progress.cpp
    truncate.cpp
    json_dump_parse.cpp
)

END()
