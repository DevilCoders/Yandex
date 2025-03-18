LIBRARY()

OWNER(velavokr)

SRCS(
    dater.cpp
    scanner.cpp
    patterns_impl.rl6
)

PEERDIR(
    kernel/segmentator/structs
    library/cpp/deprecated/dater_old
)

END()
