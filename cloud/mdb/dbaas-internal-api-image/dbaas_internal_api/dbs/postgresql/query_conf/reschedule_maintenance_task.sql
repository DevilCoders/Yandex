SELECT code.reschedule_maintenance_task(
    i_cid         => %(cid)s,
    i_config_id   => %(config_id)s,
    i_plan_ts     => %(plan_ts)s
)
