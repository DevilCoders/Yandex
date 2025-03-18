PROGRAM(snippet_kpi)

OWNER(
    alikulin
    g:snippets
)

SRCS(
    main.cpp
    options.h
    options.cpp
)

PEERDIR(
    library/cpp/digest/md5
    library/cpp/getopt
    library/cpp/string_utils/url
    mapreduce/lib
    quality/logs/parse_lib
    util/draft
)

END()
