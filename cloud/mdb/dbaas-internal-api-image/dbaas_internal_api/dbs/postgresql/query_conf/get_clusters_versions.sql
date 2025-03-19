SELECT v.cid, v.subcid, v.shard_id, v.component, v.major_version, v.minor_version, v.edition
FROM dbaas.versions v
WHERE v.cid =  ANY(%(cids)s::text[])
  AND component = %(component)s
