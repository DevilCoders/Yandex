SELECT ig.instance_group_id, ig.subcid
FROM dbaas.subclusters AS s
INNER JOIN dbaas.instance_groups AS ig USING (subcid)
WHERE s.cid = %(cid)s
