LIBRARY()

OWNER(g:cs_dev)

IF (NOT OS_WINDOWS)
    WERROR()
ENDIF()

PEERDIR(
    kernel/common_server/library/geometry
    library/cpp/logger/global
)

SRCS(
    rect_hash.cpp
)

END()
