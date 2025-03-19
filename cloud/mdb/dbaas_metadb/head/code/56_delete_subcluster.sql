CREATE OR REPLACE FUNCTION code.delete_subcluster(
    i_cid         text,
    i_subcid      text,
    i_rev         bigint
) RETURNS void AS $$
BEGIN
    DELETE FROM dbaas.pillar
     WHERE subcid = i_subcid;
    DELETE FROM dbaas.instance_groups
     WHERE subcid = i_subcid;
    DELETE FROM dbaas.kubernetes_node_groups
     WHERE subcid = i_subcid;
    DELETE FROM dbaas.subclusters
     WHERE subcid = i_subcid;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'delete_subcluster',
             jsonb_build_object(
                'subcid', i_subcid
            )
        )
    );
END;
$$ LANGUAGE plpgsql;
