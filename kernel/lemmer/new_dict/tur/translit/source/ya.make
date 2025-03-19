LIBRARY()

OWNER(g:morphology)

SET(TR_TUR_DIR ${ARCADIA_ROOT}/dict/morphdict/translit/tur)

ADD_COMPILABLE_TRANSLIT(
    ${TR_TUR_DIR}/translit.txt
    ${TR_TUR_DIR}/pentas.txt
    tur
    "-eutf8"
)

SRCS(
    GLOBAL ${BINDIR}/untranslit_trie_tur.cpp
    GLOBAL ${BINDIR}/translit_trie_tur.cpp
    GLOBAL ${BINDIR}/ngr_arr_tur.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/tur/translit/common
)

END()
