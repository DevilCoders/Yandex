PROGRAM()

OWNER(alzobnin)

SRCS(
    pure_compiler.cpp
)

PEERDIR(
    kernel/lemmer
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/getopt
    library/cpp/langmask
    library/cpp/langs
    ysite/yandex/pure
)

END()
