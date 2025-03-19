SELECT c.fqdn, c.cpu_guarantee, c.cpu_limit, c.memory_guarantee,
       c.memory_limit, c.hugetlb_limit, c.net_guarantee, c.net_limit,
       c.io_limit, c.extra_properties, c.bootstrap_cmd, c.secrets,
       c.pending_delete, c.delete_token, v.path, v.dom0_path, v.backend,
       v.read_only, v.space_guarantee, v.space_limit, v.inode_guarantee, v.inode_limit,
       vb.path IS NOT NULL AS pending_backup, c.project_id, c.managing_project_id
FROM mdb.containers c
JOIN mdb.volumes v ON (v.container = c.fqdn AND v.dom0 = c.dom0)
LEFT JOIN mdb.volume_backups vb ON (vb.container = c.fqdn AND vb.dom0 = c.dom0 AND v.path = vb.path)
WHERE
    c.dom0 = %(dom0)s
    AND c.fqdn NOT IN (SELECT container FROM mdb.transfers WHERE src_dom0 = %(dom0)s)
    AND c.fqdn NOT IN (SELECT placeholder FROM mdb.transfers WHERE dest_dom0 = %(dom0)s)
ORDER BY c.fqdn, v.path
