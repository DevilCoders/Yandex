LIBRARY()

OWNER(g:snippets)

SRCS(
    searcher.cpp
)

PEERDIR(
    kernel/snippets/urlmenu/common
    library/cpp/containers/comptrie
)

END()
