LIBRARY()

OWNER(g:morphology)

SRCS(
    GLOBAL untranslit_trie_tur.cpp
    GLOBAL translit_trie_tur.cpp
    GLOBAL ngr_arr_tur.cpp
)

PEERDIR(
    #    kernel/lemmer/new_dict/tur/translit/common
)

END()
