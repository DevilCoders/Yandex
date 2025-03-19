LIBRARY()

OWNER(g:morphology)

SRCS(
    kaz.cpp
)

PEERDIR(
    dict/morphdict/translit/kaz_official/detranslit/lib
    kernel/lemmer/new_dict/kaz/main
    kernel/lemmer/new_dict/kaz/translit
    kernel/lemmer/untranslit
)

END()
