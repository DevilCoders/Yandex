PROGRAM(example_01_ping_pong)

OWNER(
    ddoarn
    g:kikimr
)

ALLOCATOR(LF)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/actors/core
)

END()
