SELECT code.delete_alert_from_group(
    i_alert_group_id    => %(alert_group_id)s,
    i_cid               => %(cid)s,
    i_template_id       => %(template_id)s,
    i_rev               => %(rev)s
)
