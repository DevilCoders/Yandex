OWNER(
    g:base
    g:morphology
)

LIBRARY()

ARCHIVE(
    NAME data.inc
    default_decimator.lst
)

PEERDIR(
    library/cpp/archive
)

SRCS(
    factory.cpp
)

END()
