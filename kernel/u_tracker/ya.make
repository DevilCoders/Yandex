LIBRARY()

OWNER(alsafr)

SRCS(
    u_tracker.cpp
)

PEERDIR(
    kernel/factor_storage
    kernel/keyinv/hitlist
)

GENERATE_ENUM_SERIALIZATION(u_tracker.h)

END()
