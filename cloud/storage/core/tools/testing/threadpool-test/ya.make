PROGRAM(threadpool-test)

ALLOCATOR(TCMALLOC_TC)

OWNER(g:cloud-nbs)

SRCS(
    main.cpp
)

PEERDIR(
    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics

    library/cpp/getopt
)

END()
