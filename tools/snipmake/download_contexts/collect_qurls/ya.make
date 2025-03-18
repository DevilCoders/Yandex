PROGRAM(collect_qurls)

OWNER(
    steiner
    g:snippets
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/digest/md5
    library/cpp/getopt
    kernel/urlid
    mapreduce/interface
    mapreduce/lib
    quality/logs/parse_lib
    tools/snipmake/download_contexts/common
    util/draft
    library/cpp/string_utils/url
)

END()
