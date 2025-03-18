PROGRAM()

OWNER(pg)

IF (NOT OS_WINDOWS)
    ALLOCATOR(B)
ENDIF()

PEERDIR(
    library/cpp/containers/dense_hash
    library/cpp/diff
    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()
