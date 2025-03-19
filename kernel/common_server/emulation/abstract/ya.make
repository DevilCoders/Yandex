LIBRARY()

OWNER(ivanmorozov)

PEERDIR(
    kernel/common_server/abstract
    library/cpp/config/extra
)

SRCS(
    case.cpp
    manager.cpp
    GLOBAL http.cpp
)

END()
