CREATE
OR REPLACE FUNCTION code.update_kubernetes_node_group(
    i_cid                   text,
    i_kubernetes_cluster_id text,
    i_node_group_id         text,
    i_subcid                text,
    i_rev                   bigint
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.kubernetes_node_groups
    SET kubernetes_cluster_id = i_kubernetes_cluster_id,
        node_group_id = i_node_group_id
    WHERE subcid = i_subcid;

    IF
    NOT found THEN
            RAISE EXCEPTION 'Unable to find subcluster %', i_subcid
                  USING TABLE = 'dbaas.kubernetes_node_groups';
    END IF;

        PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                'update_kubernetes_node_group',
                jsonb_build_object(
                    'subcid', i_subcid,
                    'kubernetes_cluster_id', i_kubernetes_cluster_id,
                    'node_group_id', i_node_group_id
                )
            )
        );

END;
$$
LANGUAGE plpgsql;
