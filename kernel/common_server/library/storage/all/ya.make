LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage/zoo
    kernel/common_server/library/storage/fake
    kernel/common_server/library/storage/local
    kernel/common_server/library/storage/memory
    kernel/common_server/library/storage/postgres
)

IF(NOT OS_WINDOWS)
    PEERDIR (
        kernel/common_server/library/storage/ydb
    )
ENDIF()

END()
