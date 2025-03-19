LIBRARY()

OWNER(g:morphology)

SRCS(
    basic_rus.cpp
)

PEERDIR(
    kernel/lemmer/core
    kernel/lemmer/new_dict/rus/main_with_trans
)

END()
