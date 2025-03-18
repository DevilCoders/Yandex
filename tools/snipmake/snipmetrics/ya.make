OWNER(g:snippets)

LIBRARY()

SRCS(
    avecalc.cpp
    calculators.cpp
    dump.cpp
    metriclist.cpp
    pixellength.cpp
    readability.cpp
    snip2serpiter.cpp
    snipinfos.cpp
    snipiter.cpp
    snipmetrics.cpp
    wizserp.cpp
)

PEERDIR(
    contrib/libs/re2
    dict/dictutil
    kernel/lemmer
    kernel/qtree/richrequest
    kernel/snippets/archive/view
    kernel/snippets/base
    kernel/snippets/config
    kernel/snippets/qtree
    kernel/snippets/read_helper
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/titles
    kernel/snippets/titles/make_title
    kernel/snippets/util
    kernel/snippets/wordstat
    library/cpp/cgiparam
    library/cpp/charset
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    tools/snipmake/common
    tools/snipmake/serpmetrics_xml_parser
    tools/snipmake/snippet_xml_parser/cpp_writer
    yweb/autoclassif/pornoclassifier
)

GENERATE_ENUM_SERIALIZATION(metriclist.h)

END()
