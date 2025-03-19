SELECT subcid
FROM dbaas.kubernetes_node_groups
WHERE node_group_id = %(node_group_id)s
