LIBRARY()

OWNER(
    dmitryno
    g:wizard
    gotmanov
)

PEERDIR(
    kernel/gazetteer
    kernel/gazetteer/common
    kernel/lemmer
    kernel/lemmer/dictlib
    library/cpp/deprecated/iter
    library/cpp/token
    library/cpp/tokenizer
)

SRCS(
    simpletext.cpp
)

END()
