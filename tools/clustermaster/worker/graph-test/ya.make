OWNER(g:clustermaster)

LIBRARY()

SRCS(
    data.cpp
)

PEERDIR(
    tools/clustermaster/worker/lib
    tools/clustermaster/common
)

ARCHIVE(
    NAME test_data.inc
    00-12-32.sh
    crossnode.sh
    simple.sh
    simple-clustered.sh
    simple-clustered-p2.sh
    depfail.sh
    topo-sort.sh
    command.sh
    command_non_local.sh
)

END()
