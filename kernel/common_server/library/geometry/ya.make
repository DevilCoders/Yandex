LIBRARY()

OWNER(g:cs_dev)

IF(OS_WINDOWS)
    NO_WERROR()
ELSE()
    WERROR()
ENDIF()

PEERDIR(
    library/cpp/logger/global
    kernel/common_server/library/geometry/protos
    kernel/common_server/library/json
)

GENERATE_ENUM_SERIALIZATION(coord.h)

SRCS(
    rect.cpp
    coord.cpp
    polyline.cpp
)

END()
