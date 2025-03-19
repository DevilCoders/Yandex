LIBRARY()

OWNER(g:cs_dev)

GENERATE_ENUM_SERIALIZATION(structured.h)

PEERDIR(
    kernel/common_server/library/kikimr_auth
    kernel/common_server/library/storage
    kernel/common_server/library/storage/sql
    kernel/common_server/library/unistat
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_scheme
    ydb/public/sdk/cpp/client/ydb_table
    library/cpp/string_utils/base64
)

SRCS(
    GLOBAL structured.cpp
)

END()
