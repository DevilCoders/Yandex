OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    tfattr.cpp
    erfcreator.cpp
    orangeattrs.cpp
    config.cpp
    erfattrs.cpp
    catalogreader.cpp
    ifactorsreader.h
    url_based.cpp
    url_based_features.cpp
)

PEERDIR(
    contrib/libs/protobuf
    kernel/doc_remap
    kernel/erfcreator/urllen
    kernel/erfcreator/canonizers
    kernel/groupattrs
    kernel/gsk_model
    kernel/herf_hash
    kernel/indexer/faceproc
    kernel/mirrors
    kernel/news_annotations
    kernel/ngrams
    kernel/region2country
    kernel/remap
    kernel/search_types
    kernel/tarc/iface
    kernel/url
    library/cpp/containers/comptrie
    library/cpp/deprecated/dater_old
    library/cpp/deprecated/dater_old/scanner
    library/cpp/deprecated/split
    library/cpp/getopt/small
    library/cpp/microbdb
    library/cpp/on_disk/aho_corasick
    library/cpp/on_disk/chunks
    library/cpp/regex/pcre
    library/cpp/regex/pire
    ysite/yandex/dates
    ysite/yandex/erf_format
    ysite/yandex/pure
    yweb/config
    yweb/protos
    yweb/robot/dbscheeme
    yweb/robot/urlgeo_ml
    #!!!!!!include from program
    #yweb/robot/mergearc
    library/cpp/string_utils/url
)

END()

RECURSE(
    canonizers
    common_config
)
