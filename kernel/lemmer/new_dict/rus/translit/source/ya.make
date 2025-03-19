LIBRARY()

OWNER(g:morphology)

SET(TR_RUS_DIR ${ARCADIA_ROOT}/dict/morphdict/translit/rus)

PYTHON(
    ${TR_RUS_DIR}/preptbl.py ${TR_RUS_DIR}/untranslit_table.csv
    IN ${TR_RUS_DIR}/untranslit_table.csv
    STDOUT untranslit_table_rus.txt
)

ADD_COMPILABLE_TRANSLIT(
    ${BINDIR}/untranslit_table_rus.txt
    ${TR_RUS_DIR}/table.txt
    rus
    "-eutf8"
)

RUN_PROGRAM(
    dict/tools/maketransdict ruslit -i ${TR_RUS_DIR}/dict.txt
    IN ${TR_RUS_DIR}/dict.txt
    STDOUT translit.rus.cpp
)

SRCS(
    GLOBAL ${BINDIR}/untranslit_trie_rus.cpp
    GLOBAL ${BINDIR}/translit_trie_rus.cpp
    GLOBAL ${BINDIR}/ngr_arr_rus.cpp
    GLOBAL ${BINDIR}/translit.rus.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/rus/translit/common
)

END()
