PROGRAM()

OWNER(
    g:begemot
)

PEERDIR(
    library/cpp/containers/dense_hash
    library/cpp/eventlog
    library/cpp/diff
    library/cpp/getopt
    search/fields
    search/idl
    search/wizard
    search/wizard/common/core
    search/wizard/config
    search/wizard/core
    search/wizard/face
    tools/printwzrd/lib
    ysite/yandex/reqdata
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/common
)

ALLOCATOR(LF)

SRCS(
    diff_yt.cpp
)

END()
