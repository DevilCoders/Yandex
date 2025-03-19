CREATE OR REPLACE FUNCTION code.plan_managed_backup(
	i_backup_id      text,
	i_cid            text,
	i_subcid         text,
	i_shard_id       text,
	i_status         dbaas.backup_status,
	i_method         dbaas.backup_method,
	i_initiator      dbaas.backup_initiator,
	i_delayed_until  timestamptz,
	i_scheduled_date timestamp,
	i_parent_ids     text[],
	i_child_id       text,
    i_metadata 	     jsonb DEFAULT NULL::jsonb

) RETURNS dbaas.backups AS $$
DECLARE
	v_backup       dbaas.backups;
BEGIN
	INSERT INTO dbaas.backups (backup_id, cid, subcid, shard_id, status, method, initiator, delayed_until, scheduled_date, metadata)
	VALUES (i_backup_id, i_cid, i_subcid, i_shard_id, i_status, i_method, i_initiator, i_delayed_until, i_scheduled_date, i_metadata) RETURNING * INTO v_backup;

	IF i_parent_ids <> '{}' THEN
		INSERT INTO dbaas.backups_dependencies
			(parent_id, child_id)
		SELECT parent_id, i_child_id FROM unnest(i_parent_ids) parent_id;
	END IF;

	return v_backup;
END;
$$ LANGUAGE plpgsql;
