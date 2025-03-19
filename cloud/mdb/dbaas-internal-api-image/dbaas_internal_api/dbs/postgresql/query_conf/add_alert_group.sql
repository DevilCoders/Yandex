SELECT code.add_alert_group(
    i_ag_id                 => %(ag_id)s,
    i_cid                   => %(cid)s,
    i_monitoring_folder_id  => %(monitoring_folder_id)s,
    i_managed               => %(managed)s,
    i_rev                   => %(rev)s
)
