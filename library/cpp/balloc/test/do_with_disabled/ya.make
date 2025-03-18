PROGRAM()

OWNER(salmin)

ALLOCATOR(B)

CFLAGS(-fno-builtin)

NO_OPTIMIZE()
NO_UTIL()

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/balloc/setup/disable_by_default
)

END()

NEED_CHECK()
