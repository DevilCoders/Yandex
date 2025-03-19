SELECT
       monitoring_cloud_id
FROM
     dbaas.clusters
WHERE cid = %(cid)s;
