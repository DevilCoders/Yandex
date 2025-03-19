LIBRARY()

OWNER(g:snippets)

SRCS(
    hostnames.cpp
)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/archive
)


ARCHIVE(
    NAME hostnames.inc
    bold_substitute.trie
)

END()
