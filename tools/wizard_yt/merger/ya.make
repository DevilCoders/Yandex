PROGRAM()

OWNER(
    g:begemot
)

PEERDIR(
    library/cpp/getopt
    library/cpp/json/writer
    library/cpp/string_utils/base64
    library/cpp/json/fast_sax
    mapreduce/yt/interface
    mapreduce/yt/common
    mapreduce/yt/client
    tools/wizard_yt/reducer_common
)

ALLOCATOR(B)

SRCS(
    main.cpp
)

END()
