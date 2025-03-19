LIBRARY()

OWNER(g:cs_dev)

SRCS(
    replier.cpp
)

PEERDIR(
    library/cpp/cgiparam
    kernel/common_server/library/report_builder
    kernel/common_server/library/unistat
    kernel/common_server/library/openssl
    kernel/common_server/library/network/data
    search/output_context
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(replier.h)

END()
