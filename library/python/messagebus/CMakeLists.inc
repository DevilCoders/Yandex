ENABLE(MAKE_ONLY_SHARED_LIB)
SET(SHLIB_MAJOR 1)
SET(SHLIB_MINOR 0)

PEERDIR(
    library/python/messagebus/wrapper
)

SRCS (
    ${ARCADIA_ROOT}/library/python/messagebus/messagebus.swg
)
