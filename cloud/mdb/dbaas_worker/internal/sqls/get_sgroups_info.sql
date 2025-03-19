SELECT network_id, sg_type, sg_hash, sg_allow_all, array_agg(sg_ext_id) as sgroups
FROM dbaas.clusters LEFT JOIN dbaas.sgroups using(cid)
WHERE cid=%(cid)s GROUP BY network_id, sg_type, sg_hash, sg_allow_all
