LIBRARY()

OWNER(mihaild)

FROM_SANDBOX(531370785 OUT_NOAUTO eng-rus-filtered.txt)
ADD_COMPILABLE_TRANSLATE(
    eng-rus-filtered.txt
    rus
    "-eutf-8"
)

FROM_SANDBOX(531371579 OUT_NOAUTO eng-tur-filtered.txt)
ADD_COMPILABLE_TRANSLATE(
    eng-tur-filtered.txt
    tur
    "-eutf-8"
)

FROM_SANDBOX(531364796 OUT_NOAUTO eng-ukr-filtered.txt)
ADD_COMPILABLE_TRANSLATE(
    eng-ukr-filtered.txt
    ukr
    "-eutf-8"
)

SRCS(
    GLOBAL ${BINDIR}/transdict.rus.cpp
    GLOBAL ${BINDIR}/transdict.tur.cpp
    GLOBAL ${BINDIR}/transdict.ukr.cpp
)

PEERDIR(
    kernel/translate/common
)

END()
