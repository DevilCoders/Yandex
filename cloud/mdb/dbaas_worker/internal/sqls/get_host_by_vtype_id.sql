SELECT h.fqdn
FROM dbaas.hosts h
WHERE h.vtype_id = %(vtype_id)s
