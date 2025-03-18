OWNER(chelentano yuryalexkeev)

UNITTEST()

PEERDIR(
    ADDINCL tools/clustermaster/communism/lib
)

SRCDIR(${ARCADIA_ROOT})

SRCS(
    tools/clustermaster/solver/request_ut.cpp
    tools/clustermaster/solver/solver_ut.cpp
)

END()
