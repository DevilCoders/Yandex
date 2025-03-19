LIBRARY()

OWNER(g:morphology)

SRCS(
    ukr.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/ukr/main
    kernel/lemmer/new_dict/ukr/translit
    #kernel/lemmer/new_dict/ukr/translate
)

END()
