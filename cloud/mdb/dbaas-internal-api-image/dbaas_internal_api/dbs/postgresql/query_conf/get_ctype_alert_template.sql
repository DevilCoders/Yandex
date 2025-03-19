SELECT
       critical_threshold,
       warning_threshold,
       template_id,
       name,
       mandatory,
       description
FROM dbaas.default_alert
WHERE cluster_type=%(ctype)s
