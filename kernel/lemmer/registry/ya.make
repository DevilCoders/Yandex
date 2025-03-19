LIBRARY()

OWNER(
    g:base
    g:morphology
)

PEERDIR(
    library/cpp/charset
    kernel/lemmer/alpha
)

SRCS(
    language_directory.cpp
)

END()
