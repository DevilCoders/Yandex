LIBRARY()

OWNER(g:snippets)

SRCS(
    yaca_list.cpp
)

PEERDIR(
    library/cpp/regex/pire
    library/cpp/resource
    library/cpp/string_utils/url
)

ARCHIVE(
    NAME sahibinden.inc
    sahibinden.trie
    sahibinden_cars.trie
)

END()
