PROGRAM(filestore-client)

OWNER(g:cloud-nbs)

ALLOCATOR(TCMALLOC_TC)

SPLIT_DWARF()

SRCS(
    main.cpp
)

PEERDIR(
    cloud/filestore/client/lib

    library/cpp/getopt
)

END()
