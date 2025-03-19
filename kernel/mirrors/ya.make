OWNER(alexnick)

LIBRARY()

NO_WSHADOW()

SRCS(
    mirrors.cpp
    mirrors_trie.cpp
    mirrors_wrapper.cpp
    url_filter.h
)

PEERDIR(
    kernel/trie_common
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/deprecated/datafile
    library/cpp/deprecated/sgi_hash
    library/cpp/getopt/small
    library/cpp/on_disk/fried_trie
    library/cpp/on_disk/st_hash
    library/cpp/region
    library/cpp/uri
    util/draft
)

END()
