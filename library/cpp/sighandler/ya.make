LIBRARY()

OWNER(pg)

SRCS(
    async_signals_handler.cpp
    async_signals_handler.h
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()
