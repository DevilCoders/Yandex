-- Add "use_cloud_logbroker" flag to pillar for initiate switch billing maintenance task
DO
$do$
    DECLARE
        c dbaas.clusters;
        v_rev bigint;
        v_opid text;

    BEGIN
        FOR c IN
            SELECT *
            FROM dbaas.clusters
            WHERE type IN (
                'mysql_cluster',
                'kafka_cluster',
                'mongodb_cluster',
                'clickhouse_cluster',
                'greenplum_cluster',
                'redis_cluster',
                'postgresql_cluster',
                'elasticsearch_cluster'
                )  -- all except hadoop and sqlserver (they use different billing)
            AND (status = 'RUNNING' OR status = 'STOPPED')
            AND EXISTS(
                    SELECT 1
                    FROM dbaas.pillar
                    WHERE clusters.cid = pillar.cid
                      AND (value -> 'data' -> 'billing' -> 'use_cloud_logbroker') IS NULL
                )
        LOOP

            SELECT rev INTO v_rev FROM code.lock_cluster(c.cid, 'MDB-17398');

            UPDATE dbaas.pillar
            SET value = jsonb_set(value, '{data,billing}', '{"use_cloud_logbroker": false}'::jsonb)
            WHERE cid = c.cid;

            PERFORM code.complete_cluster_change(c.cid, v_rev);

            SELECT gen_random_uuid()::text INTO v_opid;

            PERFORM code.add_finished_operation_for_current_rev(
                i_operation_id => v_opid,
                i_cid => c.cid,
                i_folder_id => c.folder_id,
                i_operation_type => (c.type::text || '_modify'),
                i_metadata => '{}'::jsonb,
                i_user_id => 'migration',
                i_version => 2,
                i_hidden => true,
                i_rev => v_rev);

            -- Mark dummy operation for tracing non-updated clusters (error/modifying statuses at the time of the update)
            UPDATE dbaas.worker_queue
            SET task_args = jsonb_set(task_args, '{is_switch_billing}', 'true')
            WHERE task_id = v_opid;

        END LOOP;
    END
$do$;
