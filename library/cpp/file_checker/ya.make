LIBRARY()

OWNER(velavokr)

SRCS(
    registry_checker.h
    periodic_checker.cpp
)

PEERDIR(
    util/draft
    library/cpp/deprecated/atomic
)

END()
