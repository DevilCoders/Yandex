LIBRARY()

OWNER(gulin)

IF(DEFINED RESERVE_ADDRESS_SPACE)
    CFLAGS(-DRESERVE_ADDRESS_SPACE_${RESERVE_ADDRESS_SPACE})
ENDIF()

PEERDIR(
    library/cpp/malloc/api
)

SRCS(
    info.cpp
    hu_alloc.cpp
)

END()

RECURSE(
    link_test
)
