PROGRAM()

OWNER(
    g:begemot
)

PEERDIR(
    library/cpp/eventlog
    library/cpp/getopt
    search/fields
    search/idl
    search/wizard
    search/wizard/common/core
    search/wizard/config
    search/wizard/core
    search/wizard/face
    search/wizard/rules
    tools/printwzrd/lib
    ysite/yandex/reqdata
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/common
)

ALLOCATOR(LF)

SRCS(
    redundant_rules.cpp
)

END()
