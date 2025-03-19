INSERT INTO dbaas.alert (
    alert_group_id,
    template_id,
    critical_threshold,
    warning_threshold,
    condition,
    default_thresholds,
    status
)
SELECT
    %(alert_group_id)s,
    template_id,
    critical_threshold,
    warning_threshold,
    condition,
    true,
    'CREATING'
FROM
    dbaas.default_alert
WHERE
    cluster_type = %(cluster_type)s
