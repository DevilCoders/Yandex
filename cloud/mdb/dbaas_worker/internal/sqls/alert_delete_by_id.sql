DELETE FROM dbaas.alert WHERE alert_group_id = %(alert_group_id)s AND template_id = ANY(%(template_ids)s)
