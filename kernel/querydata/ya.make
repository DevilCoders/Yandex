OWNER(
    alexbykov
    mvel
)

RECURSE(
    cgi
    client
    common
    dump
    idl
    indexer
    request
    saas
    saas_yt
    saas_yt/idl
    scheme
    server
    tries
)

IF (NOT OS_WINDOWS)
    RECURSE(
        ut
    )
ENDIF()
