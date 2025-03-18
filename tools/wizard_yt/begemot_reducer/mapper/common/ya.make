LIBRARY()

OWNER(
    g:begemot
)

PEERDIR(
    search/begemot/server
    library/cpp/getopt
    library/cpp/string_utils/quote
    tools/wizard_yt/reducer_common
    mapreduce/yt/client
    mapreduce/yt/common
)

SRCS(
    process_row_task.cpp
    main.cpp
)

END()
