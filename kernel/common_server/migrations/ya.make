LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    contrib/libs/protobuf
    kernel/common_server/api
    kernel/common_server/api/history
    kernel/common_server/common
    kernel/common_server/library/logging
    kernel/common_server/library/storage/abstract
    kernel/common_server/library/storage/records
    kernel/common_server/library/storage/reply
    kernel/common_server/proto
    kernel/common_server/util
    kernel/common_server/util
    library/cpp/digest/md5
    library/cpp/protobuf/json
)

SRCS(
    config.cpp
    manager.cpp
    migration.cpp
    GLOBAL fake_source.cpp
    GLOBAL file_source.cpp
    GLOBAL folder_source.cpp
)

END()
