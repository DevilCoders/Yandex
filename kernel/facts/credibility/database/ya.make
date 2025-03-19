LIBRARY()

OWNER(
    g:facts
)

SRCS(
    trie_search.cpp
)

PEERDIR(
    kernel/facts/url_expansion
    kernel/hosts/owner
    kernel/querydata/client
    kernel/querydata/server
    kernel/querydata/ut_utils
    library/cpp/scheme
    library/cpp/string_utils/url
    search/meta
)

END()
