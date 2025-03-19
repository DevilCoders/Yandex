DO
$do$
    DECLARE
        c dbaas.clusters;
    BEGIN
        FOR c IN
            SELECT *
            FROM dbaas.clusters
            WHERE type = 'redis_cluster'
              AND (status = 'RUNNING' OR status = 'STOPPED')
            LOOP
                PERFORM code.add_finished_operation_for_current_rev(
                    i_operation_id => gen_random_uuid()::text,
                    i_cid => c.cid,
                    i_folder_id => c.folder_id,
                    i_operation_type => 'redis_cluster_move',
                    i_metadata => '{}'::jsonb,
                    i_user_id => 'migration',
                    i_version => 2,
                    i_hidden => true,
                    i_rev => c.actual_rev);
            END LOOP;
    END
$do$;
