PROGRAM(dump_comptrie)

OWNER(
    g:antimalware
    g:antiwebspam
)

PEERDIR(
    library/cpp/colorizer
    library/cpp/containers/comptrie
    library/cpp/getopt
)

SRCS(
    dump_trie.cpp
)

END()
