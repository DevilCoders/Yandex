UNITTEST()

SRCS(
    xml2proto_ut.cpp
)

DATA(
    arcadia/kernel/common_server/library/xml2proto/ut/data/nbki.xml
    arcadia/kernel/common_server/library/xml2proto/ut/data/book.xml
)

PEERDIR(
    kernel/common_server/library/xml2proto/ut/data
    kernel/common_server/library/xml2proto    
    library/cpp/logger/global
)

#RECURSE(data)

END()
