OWNER(g:go-library)

IF (
    OS_LINUX
    OR
    OS_DARWIN
)
    RECURSE(geobase)
ENDIF()

RECURSE(
    awstvm
    blackbox
    deploy
    laas
    nanny
    oauth
    solomon
    stq
    tvm
    uagent
    uatraits
    unistat
    webauth
    xiva
    yav
    ydb
    yplite
)
