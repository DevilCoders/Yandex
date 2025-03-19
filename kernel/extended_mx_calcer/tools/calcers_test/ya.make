PROGRAM()

OWNER(epar)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/extended_mx_calcer
    library/cpp/getopt
    library/cpp/scheme
    library/cpp/streams/factory
)

END()

RECURSE(tests)
