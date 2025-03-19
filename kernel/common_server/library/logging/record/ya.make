LIBRARY()

OWNER(g:cs_dev)

GENERATE_ENUM_SERIALIZATION(record.h)

PEERDIR(
    kernel/searchlog
    library/cpp/histogram/rt
    library/cpp/http/misc
    library/cpp/json
    library/cpp/unistat
    library/cpp/xml/document
    kernel/common_server/library/json
    kernel/common_server/library/unistat
)

SRCS(
    record.cpp
    tskv.cpp
)

END()

RECURSE_FOR_TESTS(ut)
