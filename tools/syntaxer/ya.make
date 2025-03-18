PROGRAM()

OWNER(
    udovichenko-r
    nslus
)

ALLOCATOR(LF)

SRCS(
    main.cpp
    printer.cpp
)

PEERDIR(
    dict/light_syntax/simple
    kernel/remorph/tokenizer
    library/cpp/getopt
)

END()
