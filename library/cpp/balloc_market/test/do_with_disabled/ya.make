PROGRAM()

OWNER(omakovski)

ALLOCATOR(BM)

CFLAGS(-fno-builtin)

NO_OPTIMIZE()

NO_UTIL()

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/balloc_market/setup/disable_by_default
)

END()
