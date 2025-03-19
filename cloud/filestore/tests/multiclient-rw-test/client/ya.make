PROGRAM(client)

OWNER(g:cloud-nbs)

SRCS(
    app.cpp
    main.cpp
    options.cpp
    test.cpp
)

PEERDIR(
    cloud/storage/core/libs/diagnostics
    
    library/cpp/getopt
    library/cpp/getopt/small
    library/cpp/digest/crc32c
)

END()
