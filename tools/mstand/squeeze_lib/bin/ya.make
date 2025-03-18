PROGRAM()

OWNER(
    g:mstand
)

PEERDIR(
    library/cpp/getopt
    mapreduce/yt/util
    quality/user_sessions/request_aggregate_lib
    tools/mstand/squeeze_lib/lib
    yweb/blender/common
)

SRCS(
    main.cpp
    options.cpp
)

GENERATE_ENUM_SERIALIZATION(options.h)

END()
