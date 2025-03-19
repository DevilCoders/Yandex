SELECT code.update_kubernetes_node_group(
    i_cid                   => %(cid)s,
    i_kubernetes_cluster_id => %(kubernetes_cluster_id)s,
    i_node_group_id         => %(node_group_id)s,
    i_subcid                => %(subcid)s,
    i_rev                   => %(rev)s
)
