LIBRARY()

OWNER(g:aapi)

PEERDIR(
    library/cpp/neh
    library/cpp/monlib/deprecated/json
    aapi/lib/trace
)

SRCS(
    sensors.cpp
)

END()
