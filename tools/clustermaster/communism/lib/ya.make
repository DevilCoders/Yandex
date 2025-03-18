OWNER(chelentano yuryalexkeev)

LIBRARY()

SRCDIR(tools/clustermaster/solver)

SRCS(
    batch.cpp
    request.cpp
)

PEERDIR(
    ADDINCL tools/clustermaster/communism/util
    tools/clustermaster/communism/client
)

END()
