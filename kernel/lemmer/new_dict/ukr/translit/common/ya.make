LIBRARY()

OWNER(g:morphology)

SRCS(
    translit.cpp
)

PEERDIR(
    kernel/lemmer/alpha
    kernel/lemmer/core
    kernel/lemmer/untranslit
)

END()
