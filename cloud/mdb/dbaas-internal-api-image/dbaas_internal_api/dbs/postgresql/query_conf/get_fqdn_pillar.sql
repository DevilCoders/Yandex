SELECT value
    FROM code.get_pillar_by_host(%(fqdn)s, %(target_id)s)
WHERE priority = 'fqdn'
