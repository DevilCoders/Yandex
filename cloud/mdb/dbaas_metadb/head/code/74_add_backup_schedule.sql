CREATE OR REPLACE FUNCTION code.add_backup_schedule(
    i_cid      text,
    i_rev      bigint,
    i_schedule jsonb
) RETURNS void AS $$
BEGIN
    INSERT INTO dbaas.backup_schedule (
        cid,
        schedule
    )
    VALUES (
        i_cid,
        i_schedule
    )
    ON CONFLICT (cid) DO UPDATE
    SET schedule = EXCLUDED.schedule;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_backup_schedule',
            jsonb_build_object(
                'cid', i_cid
            )
        )
    );
END;
$$ LANGUAGE plpgsql;
