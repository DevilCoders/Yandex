LIBRARY()

OWNER(g:morphology)

SET(TR_UKR_DIR ${ARCADIA_ROOT}/dict/morphdict/translit/ukr)

PYTHON(
    ${TR_UKR_DIR}/preptbl.py ${TR_UKR_DIR}/untranslit_table.csv
    IN ${TR_UKR_DIR}/untranslit_table.csv
    STDOUT untranslit_table_ukr.txt
)

ADD_COMPILABLE_TRANSLIT(
    ${BINDIR}/untranslit_table_ukr.txt
    ${TR_UKR_DIR}/table.txt
    ukr
    "-eutf8"
)

RUN_PROGRAM(
    dict/tools/maketransdict ukrlit -i ${TR_UKR_DIR}/dict.txt
    IN ${TR_UKR_DIR}/dict.txt
    STDOUT translit.ukr.cpp
)

SRCS(
    GLOBAL ${BINDIR}/untranslit_trie_ukr.cpp
    GLOBAL ${BINDIR}/translit_trie_ukr.cpp
    GLOBAL ${BINDIR}/ngr_arr_ukr.cpp
    GLOBAL ${BINDIR}/translit.ukr.cpp
)

END()
