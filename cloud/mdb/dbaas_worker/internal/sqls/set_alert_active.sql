UPDATE dbaas.alert
SET
    status = 'ACTIVE',
    alert_ext_id = %(ext_id)s
WHERE
    template_id = %(template_id)s AND
    alert_group_id = %(group_id)s
