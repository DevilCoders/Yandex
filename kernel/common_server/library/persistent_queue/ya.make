OWNER(g:cs_dev)

RECURSE(
    abstract
    fake
    logbroker
    db
    kafka
)

IF(NOT OS_WINDOWS)
    RECURSE(
        cat
        kafka
    )
ENDIF()
