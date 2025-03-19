SELECT v.cid, v.subcid, v.shard_id, v.component, v.major_version, v.minor_version, v.edition
FROM dbaas.versions_revs v
WHERE v.cid = %(cid)s
  AND v.rev = %(rev)s
  AND v.component = %(component)s
