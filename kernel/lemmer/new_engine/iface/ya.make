LIBRARY()

OWNER(g:morphology)

SRCS(
    dict_iface.cpp
)

PEERDIR(
    library/cpp/containers/comptrie
    kernel/lemmer/dictlib
)

END()
