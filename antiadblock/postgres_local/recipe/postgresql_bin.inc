BUILD_ONLY_IF(OS_LINUX OS_DARWIN ARCH_X86_64)
IF (OS_LINUX)
    DATA(
        # PostgreSQL bin 10.5.1 for linux
        sbr://750184383
        # wal2json (from postgresql-10-wal2json)
        sbr://1701149497
    )
    PEERDIR(
        contrib/libs/libpq
    )
ELSEIF (OS_DARWIN)
    DATA(
        # PostgreSQL bin 10.5.1 for Mac OS
        sbr://742548214
    )
ENDIF()
