PROGRAM()

OWNER(iddqd)

ALLOCATOR(C)

CFLAGS(-fno-builtin)

NO_OPTIMIZE()

NO_UTIL()

PEERDIR(
    library/cpp/malloc/calloc/options/slave_lf
)

SRCS(
    main.cpp
)

END()
