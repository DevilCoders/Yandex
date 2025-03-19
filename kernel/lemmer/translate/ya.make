LIBRARY()

OWNER(
    g:base
    g:morphology
)

PEERDIR(
    kernel/lemmer/dictlib
    library/cpp/streams/base64
)

SRCS(
    translatedict.cpp
)

END()
