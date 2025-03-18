PROGRAM()

ALLOCATOR(GOOGLE)

OWNER(alzobnin)

SRCS(lemmer-test.cpp)

PEERDIR(
    dict/disamb/mx_disamb
    dict/disamb/zel_disamb
    kernel/lemmer
    kernel/lemmer/dictlib
    kernel/lemmer/fixlist_load
    kernel/lemmer/tools
    kernel/search_types
    library/cpp/charset
    library/cpp/getopt
    library/cpp/langmask
    library/cpp/langs
    library/cpp/svnversion
    library/cpp/token
    library/cpp/tokenizer
)

END()
