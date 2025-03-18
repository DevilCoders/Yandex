PROGRAM(make_comptrie)

OWNER(
    g:antimalware
    g:antiwebspam
)

PEERDIR(
    kernel/hosts/owner
    library/cpp/containers/comptrie
    library/cpp/getopt
    library/cpp/uri
)

SRCS(
    make_trie.cpp
)

END()
