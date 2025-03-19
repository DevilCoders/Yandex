SELECT code.delete_shard(
    i_shard_id => %(shard_id)s,
    i_cid      => %(cid)s,
    i_rev      => %(rev)s
)
