SELECT rev
  FROM code.lock_cluster(%(cid)s, %(x_request_id)s)