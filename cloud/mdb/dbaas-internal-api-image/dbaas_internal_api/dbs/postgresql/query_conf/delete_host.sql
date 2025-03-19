SELECT fqdn,
       subcid,
       shard_id,
       space_limit,
       flavor_id AS flavor,
       vtype,
       vtype_id,
       disk_type_id,
       assign_public_ip
  FROM code.delete_hosts(
      i_fqdns => %(fqdns)s,
      i_cid   => %(cid)s,
      i_rev   => %(rev)s
  )
