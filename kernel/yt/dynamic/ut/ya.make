PROGRAM()

OWNER(
    g:fastcrawl
    g:jupiter
)

SRCS(
    ../schema_ut.cpp
    ../table_ut.cpp
    ../proto_api_ut.cpp
    lib.cpp
    main.cpp
    tables.proto
)

PEERDIR(
    kernel/proto_fuzz
    library/cpp/protobuf/util
    library/cpp/testing/unittest
    kernel/yt/dynamic
)

END()
