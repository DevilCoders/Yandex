LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/kv/abstract
    kernel/common_server/library/kv/db
    kernel/common_server/library/kv/log
)

IF (NOT OS_WINDOWS)
    PEERDIR(
        kernel/common_server/library/kv/s3
    )
ENDIF()

END()
