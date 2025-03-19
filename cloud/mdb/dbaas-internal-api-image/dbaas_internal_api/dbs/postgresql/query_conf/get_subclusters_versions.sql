SELECT s.cid, v.subcid, v.shard_id, v.component, v.major_version, v.minor_version, v.edition
FROM dbaas.versions v JOIN dbaas.subclusters s USING (subcid)
WHERE s.cid =  ANY(%(cids)s::text[])
    AND v.component = %(component)s
