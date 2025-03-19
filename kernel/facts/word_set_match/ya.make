LIBRARY()

OWNER(
    g:facts
)

SRCS(
    word_set_match.cpp
)

PEERDIR(
    kernel/facts/common
    library/cpp/on_disk/aho_corasick
)

END()
