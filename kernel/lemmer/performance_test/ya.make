PROGRAM()

OWNER(g:morphology)

SRCS(
    main.cpp
)

FROM_SANDBOX(FILE 560145177 OUT_NOAUTO input_sample.txt)

FROM_SANDBOX(FILE 560145854 OUT_NOAUTO input_one.txt)

PEERDIR(
    library/cpp/charset
    kernel/lemmer/core
    library/cpp/getopt
    library/cpp/tokenizer
)

END()
