OWNER(
    lagrunge
    sobols
    suns
)

LIBRARY()

SRCS(
    pusher.cpp
)

PEERDIR(
    library/cpp/http/simple
    library/cpp/json
    library/cpp/logger/global
    library/cpp/unistat
)

END()
