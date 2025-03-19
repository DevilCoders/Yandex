SELECT max(rev) rev
  FROM dbaas.clusters_changes
 WHERE cid = %(cid)s
   AND committed_at <= %(timestamp)s
