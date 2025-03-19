OWNER(g:base)

UNITTEST()

PEERDIR(
    ADDINCL kernel/indexer/tfproc
)

SRCDIR(kernel/indexer/tfproc)

SRCS(
    inthash_ut.cpp
    titleproc_ut.cpp
    date_recognizer_ut.cpp
)

END()
