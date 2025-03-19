SELECT code.add_alert_to_group (
    i_cid                   => %(cid)s,
	i_alert_group_id        => %(alert_group_id)s,
	i_template_id           => %(template_id)s,
    i_notification_channels => %(notification_channels)s,
    i_disabled              => %(disabled)s,
	i_crit_threshold        => %(critical_threshold)s,
	i_warn_threshold        => %(warning_threshold)s,
    i_default_thresholds    => %(default_thresholds)s,
    i_rev                   => %(rev)s
);
