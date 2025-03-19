LIBRARY()

OWNER(g:morphology)

SRCS(
    rus.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/rus/main
    kernel/lemmer/new_dict/rus/translit
    #kernel/lemmer/new_dict/rus/translate
)

END()
