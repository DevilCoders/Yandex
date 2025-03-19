SELECT * FROM code.plan_managed_backup(
	i_backup_id      => %(backup_id)s,
	i_cid            => %(cid)s,
	i_subcid         => %(subcid)s,
	i_shard_id       => %(shard_id)s,
	i_status         => 'PLANNED'::dbaas.backup_status,
	i_method         => %(backup_method)s::dbaas.backup_method,
	i_initiator      => 'USER'::dbaas.backup_initiator,
	i_delayed_until  => NOW(),
    i_scheduled_date => NULL,
    i_parent_ids     => %(parent_ids)s,
    i_child_id       => %(backup_id)s
)
