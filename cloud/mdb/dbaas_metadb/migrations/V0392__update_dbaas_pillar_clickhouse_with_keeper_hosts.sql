UPDATE dbaas.pillar pi
SET value = jsonb_set(
	pi.value,
	'{data,clickhouse,keeper_hosts}', 
 	t.new_value
)
FROM (
	SELECT
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
) AS t
WHERE pi.subcid = t.subcid;