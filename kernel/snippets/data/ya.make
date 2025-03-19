LIBRARY()

OWNER(g:snippets)

SRCS(
    abbrev_words.h
    abbrev_words.cpp
    pornodict.h
    pornodict.cpp
)

PEERDIR(
    library/cpp/containers/comptrie
)

END()
