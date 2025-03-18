PROGRAM()

OWNER(alzobnin)

ALLOCATOR(LF)

SRCS(
    main.cpp
    printer.cpp
)

PEERDIR(
    kernel/inflectorlib/phrase/simple
    library/cpp/getopt
)

END()

RECURSE(tests)
