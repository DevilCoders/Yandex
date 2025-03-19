UNITTEST()

SIZE(SMALL)

OWNER(g:cs_dev)

SRCS(
    structured_ut.cpp
)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    kernel/common_server/library/storage/ydb
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

END()
