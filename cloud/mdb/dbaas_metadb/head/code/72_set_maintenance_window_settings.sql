CREATE OR REPLACE FUNCTION code.set_maintenance_window_settings(
    i_cid   text,
    i_day   dbaas.maintenance_window_days,
    i_hour  int,
    i_rev   bigint
) RETURNS void AS $$
BEGIN
    DELETE FROM dbaas.maintenance_window_settings
        WHERE cid = i_cid;

    IF i_day IS NOT NULL AND i_hour IS NOT NULL THEN
        INSERT INTO dbaas.maintenance_window_settings (cid, day, hour)
            VALUES (i_cid, i_day, i_hour);
        PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                'set_maintenance_window_settings',
                jsonb_build_object(
                    'cid', i_cid,
                    'day', i_day,
                    'hour', i_hour
                )
            )
        );
    ELSIF i_day IS NULL AND i_hour IS NULL THEN
        PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'clear_maintenance_window_settings',
                    jsonb_build_object(
                            'cid', i_cid
                        )
                )
        );
    ELSE
        RAISE EXCEPTION 'unable to update maintenance settings if one of it is NULL';
    END IF;
END;
$$ LANGUAGE plpgsql;
