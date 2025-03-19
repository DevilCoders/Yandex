LIBRARY()

OWNER(mihaild)

SRCS(
    GLOBAL transdict.rus.cpp
    GLOBAL transdict.tur.cpp
    GLOBAL transdict.ukr.cpp
)

PEERDIR(
    kernel/translate/common
)

END()
