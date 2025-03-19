LIBRARY()

OWNER(g:morphology)

SRCS(
    GLOBAL untranslit_trie_kaz.cpp
    GLOBAL translit_trie_kaz.cpp
    GLOBAL ngr_arr_kaz.cpp
)

PEERDIR(
    #    kernel/lemmer/new_dict/kaz/translit/common
)

END()
