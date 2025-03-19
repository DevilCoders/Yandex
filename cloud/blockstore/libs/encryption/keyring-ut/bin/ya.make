UNITTEST_FOR(cloud/blockstore/libs/encryption)

OWNER(g:cloud-nbs)

IF (OS_LINUX)
    SRCS(
        keyring_ut.cpp
    )
ENDIF()

END()
