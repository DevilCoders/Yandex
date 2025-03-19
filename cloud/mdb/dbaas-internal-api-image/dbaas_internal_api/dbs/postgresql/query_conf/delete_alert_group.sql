SELECT code.delete_alert_group(
    i_alert_group_id               => %(alert_group_id)s,
    i_cid                          => %(cid)s,
    i_rev                          => %(rev)s,
    i_force_managed_group_deletion => %(force_managed_group_deletion)s
)
