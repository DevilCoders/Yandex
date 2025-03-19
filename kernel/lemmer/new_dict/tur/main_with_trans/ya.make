LIBRARY()

OWNER(g:morphology)

SRCS(
    tur.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/tur/main
    kernel/lemmer/new_dict/tur/translit
    #kernel/lemmer/new_dict/tur/translate
)

END()
