LIBRARY()

OWNER(
    g:geotargeting
)

NO_COMPILER_WARNINGS()

SRCS(
    internal_struct.cpp
    lookup.cpp
)

PEERDIR(
    library/cpp/ipreg
    library/cpp/json
)

END()
