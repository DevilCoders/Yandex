PROGRAM(tools_rcgtest)

ALLOCATOR(GOOGLE)

OWNER(alzobnin)

SRCS(
    main.cpp
    recognizers_shell.cpp
)

PEERDIR(
    dict/recognize/docrec
    kernel/recshell
    library/cpp/getopt
)

END()

RECURSE(tests)
