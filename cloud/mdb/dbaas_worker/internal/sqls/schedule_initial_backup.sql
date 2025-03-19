SELECT * FROM code.plan_managed_backup(
	i_backup_id      => %(backup_id)s,
	i_cid            => %(cid)s,
	i_subcid         => %(subcid)s,
	i_shard_id       => %(shard_id)s,
	i_status         => 'PLANNED'::dbaas.backup_status,
	i_method         => 'FULL'::dbaas.backup_method,
	i_initiator      => 'SCHEDULE'::dbaas.backup_initiator,
	i_delayed_until  => NOW() + '%(delay_seconds)s'::interval,
	i_scheduled_date => DATE(NOW() + '%(delay_seconds)s'::interval),
	i_parent_ids     => '{}',
	i_child_id=>NULL
)
