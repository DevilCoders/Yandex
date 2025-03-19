LIBRARY()

OWNER(myutman)

SRCS(
    lib.h
    lib.cpp
    distance.h
    distance.cpp
    structures.h
    structures.cpp
    restore.h
    restore.cpp
    naive_dp.h
    naive_dp.cpp
)

PEERDIR(
    library/cpp/json
)

END()