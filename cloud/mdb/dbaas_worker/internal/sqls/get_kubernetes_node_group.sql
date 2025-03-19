SELECT kubernetes_cluster_id, node_group_id
FROM dbaas.kubernetes_node_groups
WHERE subcid = %(subcid)s
