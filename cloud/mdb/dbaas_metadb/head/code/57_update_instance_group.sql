CREATE OR REPLACE FUNCTION code.update_instance_group(
    i_cid                 text,
    i_instance_group_id   text,
    i_subcid              text,
    i_rev                 bigint
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.instance_groups
    SET instance_group_id = i_instance_group_id
    WHERE subcid = i_subcid;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find subcluster %', i_subcid
              USING TABLE = 'dbaas.instance_groups';
    END IF;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_instance_group',
            jsonb_build_object(
                'subcid', i_subcid,
                'instance_group_id', i_instance_group_id
            )
        )
    );

END;
$$ LANGUAGE plpgsql;
