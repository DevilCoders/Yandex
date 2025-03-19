LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    contrib/libs/libpqxx
    kernel/common_server/library/logging
    kernel/common_server/library/storage
    kernel/common_server/library/storage/balancing
    kernel/common_server/library/storage/reply
    kernel/common_server/library/storage/sql
    kernel/common_server/library/unistat
    kernel/common_server/util
    kernel/common_server/util/logging
    kernel/daemon/common
    library/cpp/archive
    library/cpp/digest/md5
    library/cpp/json
    library/cpp/string_utils/base64
    library/cpp/testing/unittest
)

GENERATE_ENUM_SERIALIZATION(table_accessor.h)

SRCS(
    GLOBAL config.cpp
    GLOBAL postgres_storage.cpp
    GLOBAL table_accessor.cpp
    postgres_conn_pool.cpp
)

ARCHIVE(
    NAME sql.inc
    commands/alter_table_unversioned.sql
    commands/alter_table_versioned.sql
    commands/check_additional_table_exist.sql
    commands/check_exists_node.sql
    commands/check_table_exist.sql
    commands/create_additional_table.sql
    commands/create_main_table.sql
    commands/delete_history.sql
    commands/delete_test_keys.sql
    commands/get_by_key.sql
    commands/get_nodes.sql
    commands/get_value.sql
    commands/get_value_by_version.sql
    commands/insert_test_key.sql
    commands/insert_unversioned_value.sql
    commands/insert_version.sql
    commands/insert_versioned_value.sql
    commands/remove_node.sql
)

END()
