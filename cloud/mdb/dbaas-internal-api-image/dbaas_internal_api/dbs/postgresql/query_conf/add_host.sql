SELECT subcid, shard_id, space_limit,
       flavor_id AS flavor,
       geo, fqdn, vtype, disk_type_id,
       subnet_id, assign_public_ip
  FROM code.add_host(
    i_subcid => %(subcid)s,
    i_shard_id => %(shard_id)s,
    i_space_limit => %(space_limit)s,
    i_flavor_id => %(flavor)s,
    i_geo => %(geo)s,
    i_fqdn => %(fqdn)s,
    i_disk_type => %(disk_type_id)s,
    i_subnet_id => %(subnet_id)s,
    i_assign_public_ip => %(assign_public_ip)s,
    i_cid => %(cid)s,
    i_rev => %(rev)s
)
