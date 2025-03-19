LIBRARY()

OWNER(ivanmorozov)

PEERDIR(
    kernel/common_server/abstract
    kernel/common_server/emulation/abstract
)

SRCS(
    GLOBAL configured.cpp
)

END()
