DO
$do$
    DECLARE
		i_folder_id bigint;
		i_cid text;
		i_type dbaas.cluster_type;
		i_subcid text;
		i_keeper_hosts jsonb;
        v_rev bigint;
        v_opid text;

    BEGIN
        FOR i_folder_id, i_cid, i_type, i_subcid, i_keeper_hosts IN
			SELECT
				c.folder_id,
				c.cid,
				c.type,
				p.subcid,
				(
					CASE
						WHEN jsonb_array_length(p.value->'data'->'clickhouse'->'zk_hosts') > 0
							THEN (
								SELECT jsonb_object_agg(k, cast(v AS INT)) FROM unnest(
									array(SELECT jsonb_array_elements_text(p.value->'data'->'clickhouse'->'zk_hosts') AS x ORDER BY x),
									array(SELECT n FROM generate_series(1, jsonb_array_length(p.value->'data'->'clickhouse'->'zk_hosts')) AS n)
								) AS x(k, v)
							)
						ELSE '{}'::jsonb
					END
				) AS new_value
			FROM dbaas.clusters c
			LEFT JOIN dbaas.subclusters sc USING (cid)
			LEFT JOIN dbaas.pillar p USING (subcid)
			WHERE c.type = 'clickhouse_cluster' AND
				'clickhouse_cluster' = any(sc.roles) AND
				p.value#>'{data,clickhouse,zk_hosts}' IS NOT NULL
        LOOP

            SELECT rev INTO v_rev FROM code.lock_cluster(i_cid, 'MDB-17804');

			UPDATE dbaas.pillar
			SET value = jsonb_set(value, '{data,clickhouse,keeper_hosts}', i_keeper_hosts)
			WHERE subcid = i_subcid;

            PERFORM code.complete_cluster_change(i_cid, v_rev);

            SELECT gen_random_uuid()::text INTO v_opid;

            PERFORM code.add_finished_operation_for_current_rev(
                i_operation_id => v_opid,
                i_cid => i_cid,
                i_folder_id => i_folder_id,
                i_operation_type => (i_type::text || '_modify'),
                i_metadata => '{}'::jsonb,
                i_user_id => 'migration',
                i_version => 2,
                i_hidden => true,
                i_rev => v_rev);

        END LOOP;
    END
$do$;