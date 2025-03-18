PROGRAM(query_comptrie)

OWNER(
    g:antimalware
    g:antiwebspam
)

PEERDIR(
    library/cpp/infected_masks
    library/cpp/getopt
)

SRCS(
    query_trie.cpp
)

GENERATE_ENUM_SERIALIZATION(mode.h)

END()
