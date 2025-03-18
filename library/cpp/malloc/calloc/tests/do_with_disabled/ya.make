PROGRAM()

OWNER(iddqd)

ALLOCATOR(C)

CFLAGS(-fno-builtin)

NO_OPTIMIZE()

NO_UTIL()

PEERDIR(
    library/cpp/malloc/calloc/options/disable_by_default
)

SRCS(
    main.cpp
)

END()
