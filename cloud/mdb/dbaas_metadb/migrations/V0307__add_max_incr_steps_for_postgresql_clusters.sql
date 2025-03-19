DO
$do$
	DECLARE
		i_cid TEXT;
		i_rev BIGINT;
	BEGIN
		FOR i_cid IN
			SELECT 
				cid
			FROM 
				dbaas.clusters
			WHERE type = 'postgresql_cluster' AND (status = 'RUNNING' OR status = 'STOPPED')
		LOOP
			i_rev = (SELECT rev FROM code.lock_cluster(i_cid));
			UPDATE dbaas.backup_schedule SET schedule = jsonb_set(schedule, '{max_incremental_steps}', '6') WHERE cid = i_cid;
			PERFORM code.complete_cluster_change(i_cid, i_rev);
		END LOOP;
	END
$do$;
