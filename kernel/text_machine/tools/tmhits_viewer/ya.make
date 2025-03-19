PROGRAM()
OWNER(grechnik)
SRCS(tmhits_viewer.cpp hits_map.cpp)
RESOURCE(dochits.html /dochits.html favicon.png /favicon.png)
PEERDIR(
    kernel/reqbundle
    kernel/text_machine/proto
    kernel/text_machine/util
    library/cpp/getopt
    library/cpp/html/escape
    library/cpp/http/server
    library/cpp/resource
    library/cpp/string_utils/base64
)
GENERATE_ENUM_SERIALIZATION(hits_map.h)
END()
