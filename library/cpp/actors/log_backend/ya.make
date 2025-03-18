LIBRARY()

PEERDIR(
    library/cpp/actors/core
    library/cpp/logger
)

OWNER(
    galaxycrab
    g:kikimr
)

SRCS(
    actor_log_backend.cpp
)

END()
