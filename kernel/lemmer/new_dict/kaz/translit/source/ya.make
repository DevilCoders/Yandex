LIBRARY()

OWNER(g:morphology)

SET(TR_KAZ_DIR ${ARCADIA_ROOT}/dict/morphdict/translit/kaz)

ADD_COMPILABLE_TRANSLIT(
    ${TR_KAZ_DIR}/translit.txt
    ${TR_KAZ_DIR}/pentas.txt
    kaz
    "-eutf8"
)

SRCS(
    GLOBAL ${BINDIR}/untranslit_trie_kaz.cpp
    GLOBAL ${BINDIR}/translit_trie_kaz.cpp
    GLOBAL ${BINDIR}/ngr_arr_kaz.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/kaz/translit/common
)

END()
