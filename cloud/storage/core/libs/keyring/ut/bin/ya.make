UNITTEST_FOR(cloud/storage/core/libs/keyring)

OWNER(g:cloud-nbs)

IF (OS_LINUX)
    SRCS(
        endpoints_ut.cpp
        keyring_ut.cpp
    )
ENDIF()

END()
