PROGRAM()

OWNER(
    insight
    agusakov
    g:neural-search
)

SRCS(
    main.cpp
    grads.cpp
)

PEERDIR(
    library/cpp/getopt
    library/cpp/logger/global
    kernel/dssm_applier/nn_applier/lib
)

ALLOCATOR(LF)

END()
