LIBRARY()

OWNER(
    g:clustermaster
)

SRCS(
    data.cpp
)

PEERDIR(
    ADDINCL tools/clustermaster/master/lib
    ADDINCL tools/clustermaster/common
)

ARCHIVE(
    NAME test_data.inc
    12-20.sh
    cm_robmerge_tree_stt.host.cfg
    cm_robmerge_tree_stt.sh
    cm_multi_cluster_dep_stt.host.cfg
    cm_multi_cluster_dep_stt.sh
    cron_subgraph.sh
    crossnode-p2.sh
    crossnode-some.sh
    crossnode.sh
    comments.sh
    comments.list
    depend-on-prev-caret.sh
    depend-on-prev-default.sh
    depends-on-two.sh
    different-order.sh
    gather-non-uniform.host.cfg
    gather-non-uniform.sh
    gather.sh
    group-int-param.host.cfg
    group-int-param.sh
    group.host.cfg
    group.sh
    local-force-gather.sh
    local-force-scatter.sh
    local-multiple.sh
    local-not-gather.sh
    local-not-scatter.sh
    local.sh
    scatter-non-uniform.host.cfg
    scatter-non-uniform.sh
    scatter.sh
    undectected-gather.host.cfg
    undectected-gather.sh
    command-simple.sh
    topo_sort.sh
    fake-states.sh
    fake-states.list
)

END()
