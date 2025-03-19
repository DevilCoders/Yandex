SELECT
    access_secret, type
FROM
    dbaas.config_host_access_ids
WHERE
    access_id = %(access_id)s
    AND active = true
