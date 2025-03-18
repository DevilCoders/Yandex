PROGRAM()

OWNER(alzobnin)

SET(MAKE_RUS_TRANSLIT_FROM_SOURCE YES)

PEERDIR(
    kernel/indexer/attrproc
    kernel/lemmer
    kernel/lemmer/untranslit
    library/cpp/charset
    library/cpp/getopt
    library/cpp/tokenizer
    library/cpp/wordpos
    ysite/yandex/common
)

SRCS(
    untranslit_test.cpp
)

END()
