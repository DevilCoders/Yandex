PROGRAM()

OWNER(
    g:begemot
)

PEERDIR(
    library/cpp/eventlog
    library/cpp/getopt
    library/cpp/openssl/crypto
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
    library/cpp/cgiparam
)

ALLOCATOR(LF)

SRCS(
    cache_hit.cpp
)

END()
