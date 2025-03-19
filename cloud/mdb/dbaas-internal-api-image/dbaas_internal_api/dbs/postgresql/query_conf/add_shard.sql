SELECT subcid, shard_id, name
  FROM code.add_shard(
    i_subcid   => %(subcid)s,
    i_shard_id => %(shard_id)s,
    i_name     => %(name)s,
    i_cid      => %(cid)s,
    i_rev      => %(rev)s
)
